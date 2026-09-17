#!/usr/bin/env python3
"""Apt-Repository fuer die Fotobox pflegen, ohne dpkg- oder apt-Werkzeuge.

Layout des Repos (Wurzel = GitHub-Pages-Site):
  pool/<dist>/<component>/<paket>.deb
  dists/<dist>/<component>/binary-<arch>/Packages, Packages.gz, Release
  dists/<dist>/Release, InRelease, Release.gpg   (signiert, wenn ein GPG-Schluessel da ist)
  fotobox.gpg                                    (oeffentlicher Schluessel)
  index.html

Befehle:
  add       --repo DIR --dist trixie --component nightly PAKET.deb ...
  generate  --repo DIR [--dists bookworm trixie] [--keep-nightly 5] [--gpg-key ID] [--base-url URL]
"""
import argparse
import gzip
import hashlib
import io
import os
import re
import shutil
import subprocess
import sys
import tarfile
import time
from email.utils import formatdate
from pathlib import Path

COMPONENTS = ["main", "nightly"]


# ---------------------------------------------------------------- Debian-Versionsvergleich

def _order(c: str) -> int:
    if c == "~":
        return -1
    if c.isdigit():
        return 0
    if c.isalpha():
        return ord(c)
    return ord(c) + 256


def _verrevcmp(a: str, b: str) -> int:
    """Nachbildung von dpkgs verrevcmp()."""
    i = j = 0
    while i < len(a) or j < len(b):
        first_diff = 0
        while (i < len(a) and not a[i].isdigit()) or (j < len(b) and not b[j].isdigit()):
            ac = _order(a[i]) if i < len(a) else 0
            bc = _order(b[j]) if j < len(b) else 0
            if ac != bc:
                return ac - bc
            if i < len(a):
                i += 1
            if j < len(b):
                j += 1
        while i < len(a) and a[i] == "0":
            i += 1
        while j < len(b) and b[j] == "0":
            j += 1
        while i < len(a) and a[i].isdigit() and j < len(b) and b[j].isdigit():
            if not first_diff:
                first_diff = ord(a[i]) - ord(b[j])
            i += 1
            j += 1
        if i < len(a) and a[i].isdigit():
            return 1
        if j < len(b) and b[j].isdigit():
            return -1
        if first_diff:
            return first_diff
    return 0


def compare_versions(a: str, b: str) -> int:
    def split(v):
        epoch, _, rest = v.partition(":") if ":" in v else ("0", "", v)
        upstream, _, revision = rest.rpartition("-") if "-" in rest else (rest, "", "")
        return int(epoch), upstream, revision

    ea, ua, ra = split(a)
    eb, ub, rb = split(b)
    if ea != eb:
        return ea - eb
    r = _verrevcmp(ua, ub)
    if r != 0:
        return r
    return _verrevcmp(ra, rb)


# ---------------------------------------------------------------- .deb lesen

def parse_control(text: str) -> list:
    """Liefert [(Feld, Wert)] in Dateireihenfolge, Fortsetzungszeilen inklusive."""
    fields = []
    for line in text.splitlines():
        if not line.strip():
            continue
        if line[0] in " \t" and fields:
            k, v = fields[-1]
            fields[-1] = (k, v + "\n" + line)
        else:
            k, _, v = line.partition(":")
            fields.append((k.strip(), v.strip()))
    return fields


def read_control(deb: Path) -> list:
    data = deb.read_bytes()
    if data[:8] != b"!<arch>\n":
        raise ValueError(f"{deb}: kein ar-Archiv")
    pos = 8
    while pos + 60 <= len(data):
        hdr = data[pos : pos + 60]
        name = hdr[:16].decode("ascii", "replace").strip().rstrip("/")
        size = int(hdr[48:58].decode().strip())
        pos += 60
        body = data[pos : pos + size]
        pos += size + (size % 2)
        if not name.startswith("control.tar"):
            continue
        if name.endswith(".zst"):
            # tarfile kann kein zstd: auf dpkg-deb ausweichen, falls vorhanden.
            out = subprocess.run(["dpkg-deb", "-f", str(deb)], capture_output=True, text=True, check=True)
            return parse_control(out.stdout)
        with tarfile.open(fileobj=io.BytesIO(body), mode="r:*") as tf:
            for m in tf.getmembers():
                if m.name.lstrip("./") == "control":
                    return parse_control(tf.extractfile(m).read().decode("utf-8"))
    raise ValueError(f"{deb}: keine control-Datei gefunden")


def field(fields: list, key: str, default: str = "") -> str:
    for k, v in fields:
        if k.lower() == key.lower():
            return v
    return default


def hashes(path: Path) -> dict:
    h = {"MD5Sum": hashlib.md5(), "SHA1": hashlib.sha1(), "SHA256": hashlib.sha256()}
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            for x in h.values():
                x.update(chunk)
    return {k: v.hexdigest() for k, v in h.items()}


# ---------------------------------------------------------------- Befehle

def cmd_add(args) -> int:
    dest = Path(args.repo) / "pool" / args.dist / args.component
    dest.mkdir(parents=True, exist_ok=True)
    for deb in args.debs:
        src = Path(deb)
        read_control(src)  # validiert das Paket
        shutil.copy2(src, dest / src.name)
        print(f"hinzugefuegt: {dest / src.name}")
    return 0


def prune_nightlies(pool: Path, keep: int) -> None:
    for dist_dir in sorted(pool.glob("*")):
        nightly = dist_dir / "nightly"
        if not nightly.is_dir():
            continue
        by_name = {}
        for deb in nightly.glob("*.deb"):
            f = read_control(deb)
            by_name.setdefault(field(f, "Package"), []).append((field(f, "Version"), deb))
        for name, entries in by_name.items():
            entries.sort(key=lambda e: _cmp_key(e[0]))
            for version, deb in entries[:-keep] if keep > 0 else []:
                deb.unlink()
                print(f"entfernt (alt): {deb}")


class _cmp_key:
    def __init__(self, v):
        self.v = v

    def __lt__(self, other):
        return compare_versions(self.v, other.v) < 0


def write_packages(repo: Path, dist: str, component: str) -> dict:
    """Schreibt Packages-Dateien fuer alle Architekturen einer Komponente.
    Gibt {arch: [(relpath, size, hashes)]} fuer die Release-Datei zurueck."""
    pool = repo / "pool" / dist / component
    entries_by_arch = {}
    for deb in sorted(pool.glob("*.deb")) if pool.is_dir() else []:
        f = read_control(deb)
        arch = field(f, "Architecture", "all")
        h = hashes(deb)
        lines = [f"{k}: {v}" for k, v in f]
        lines += [
            f"Filename: {deb.relative_to(repo).as_posix()}",
            f"Size: {deb.stat().st_size}",
            f"MD5sum: {h['MD5Sum']}",
            f"SHA1: {h['SHA1']}",
            f"SHA256: {h['SHA256']}",
        ]
        entries_by_arch.setdefault(arch, []).append("\n".join(lines) + "\n")

    archs = sorted(set(entries_by_arch) | {"arm64"})
    index_files = {}
    for arch in archs:
        d = repo / "dists" / dist / component / f"binary-{arch}"
        d.mkdir(parents=True, exist_ok=True)
        text = "\n".join(entries_by_arch.get(arch, []))
        (d / "Packages").write_text(text, encoding="utf-8")
        with gzip.GzipFile(d / "Packages.gz", "wb", mtime=0) as gz:
            gz.write(text.encode("utf-8"))
        (d / "Release").write_text(
            f"Archive: {dist}\nComponent: {component}\nOrigin: Fotobox\nLabel: Fotobox\n"
            f"Architecture: {arch}\n",
            encoding="utf-8",
        )
        index_files[arch] = [d / "Packages", d / "Packages.gz", d / "Release"]
    return index_files


def write_release(repo: Path, dist: str, index_files: list, gpg_key: str, passphrase: str) -> None:
    dist_dir = repo / "dists" / dist
    archs = sorted({p.parent.name.replace("binary-", "") for p in index_files})
    lines = [
        "Origin: Fotobox",
        "Label: Fotobox",
        f"Suite: {dist}",
        f"Codename: {dist}",
        f"Architectures: {' '.join(archs)}",
        f"Components: {' '.join(COMPONENTS)}",
        f"Description: Fotobox-Pakete fuer Raspberry Pi OS {dist}",
        f"Date: {formatdate(time.time(), usegmt=True)}",
    ]
    for algo in ("MD5Sum", "SHA1", "SHA256"):
        lines.append(f"{algo}:")
        for p in index_files:
            h = hashes(p)[algo]
            lines.append(f" {h} {p.stat().st_size:>16} {p.relative_to(dist_dir).as_posix()}")
    release = dist_dir / "Release"
    release.write_text("\n".join(lines) + "\n", encoding="utf-8")

    for stale in ("InRelease", "Release.gpg"):
        (dist_dir / stale).unlink(missing_ok=True)

    if gpg_key:
        base = ["gpg", "--batch", "--yes", "--local-user", gpg_key]
        if passphrase:
            base += ["--pinentry-mode", "loopback", "--passphrase-fd", "0"]
        inp = passphrase or None
        subprocess.run(base + ["--armor", "--detach-sign", "-o", str(dist_dir / "Release.gpg"), str(release)],
                       input=inp, text=True, check=True)
        subprocess.run(base + ["--clearsign", "-o", str(dist_dir / "InRelease"), str(release)],
                       input=inp, text=True, check=True)
        print(f"signiert: dists/{dist}")


def write_index(repo: Path, dists: list, base_url: str, signed: bool) -> None:
    key_hint = (
        f"curl -fsSL {base_url}/fotobox.gpg | sudo tee /etc/apt/keyrings/fotobox.gpg >/dev/null"
        if signed else "# Repo ist nicht signiert, daher trusted=yes"
    )
    opts = "arch=arm64 signed-by=/etc/apt/keyrings/fotobox.gpg" if signed else "arch=arm64 trusted=yes"
    pkgs = []
    for dist in dists:
        for comp in COMPONENTS:
            for deb in sorted((repo / "pool" / dist / comp).glob("*.deb")):
                pkgs.append(f'<li><a href="{deb.relative_to(repo).as_posix()}">{deb.name}</a> ({dist}/{comp})</li>')
    html = f"""<!doctype html>
<html lang="de"><head><meta charset="utf-8"><title>Fotobox Apt-Repository</title>
<style>body{{font-family:system-ui,sans-serif;max-width:800px;margin:40px auto;padding:0 16px;line-height:1.5}}
pre{{background:#f4f4f4;padding:12px;overflow:auto}}</style></head><body>
<h1>Fotobox Apt-Repository</h1>
<p>Pakete fuer Raspberry Pi OS (arm64). Distributionen: {', '.join(dists)}. Komponenten: <code>main</code> (Releases), <code>nightly</code>.</p>
<h2>Einrichten auf dem Pi</h2>
<pre>sudo mkdir -p /etc/apt/keyrings
{key_hint}
. /etc/os-release
echo "deb [{opts}] {base_url} $VERSION_CODENAME main" | sudo tee /etc/apt/sources.list.d/fotobox.list
sudo apt update &amp;&amp; sudo apt install fotobox</pre>
<p>Fuer Nightlies zusaetzlich die Komponente <code>nightly</code> in die Zeile aufnehmen.</p>
<h2>Pakete</h2><ul>{''.join(pkgs) or '<li>noch keine</li>'}</ul>
<p>Erzeugt am {time.strftime('%Y-%m-%d %H:%M UTC', time.gmtime())}.</p>
</body></html>
"""
    (repo / "index.html").write_text(html, encoding="utf-8")
    (repo / ".nojekyll").write_text("")


def cmd_generate(args) -> int:
    repo = Path(args.repo)
    pool = repo / "pool"
    pool.mkdir(parents=True, exist_ok=True)
    prune_nightlies(pool, args.keep_nightly)

    dists = sorted(set(args.dists or []) | {p.name for p in pool.glob("*") if p.is_dir()})
    if not dists:
        print("keine Distributionen im Pool und keine per --dists angegeben", file=sys.stderr)
        return 1

    gpg_key = args.gpg_key or os.environ.get("APT_GPG_KEY_ID", "")
    passphrase = os.environ.get("APT_GPG_PASSPHRASE", "")
    if gpg_key and shutil.which("gpg") is None:
        print("gpg nicht gefunden, Repo bleibt unsigniert", file=sys.stderr)
        gpg_key = ""

    if (repo / "dists").exists():
        shutil.rmtree(repo / "dists")
    for dist in dists:
        index_files = []
        for comp in COMPONENTS:
            for files in write_packages(repo, dist, comp).values():
                index_files += files
        write_release(repo, dist, index_files, gpg_key, passphrase)
        print(f"dists/{dist}: {sum(1 for _ in (pool / dist).rglob('*.deb')) if (pool / dist).exists() else 0} Pakete")

    if gpg_key:
        with (repo / "fotobox.gpg").open("wb") as f:
            subprocess.run(["gpg", "--batch", "--export", gpg_key], stdout=f, check=True)
    else:
        (repo / "fotobox.gpg").unlink(missing_ok=True)

    write_index(repo, dists, args.base_url.rstrip("/"), bool(gpg_key))
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)

    a = sub.add_parser("add", help="Pakete in den Pool kopieren")
    a.add_argument("--repo", required=True)
    a.add_argument("--dist", required=True)
    a.add_argument("--component", required=True, choices=COMPONENTS)
    a.add_argument("debs", nargs="+")
    a.set_defaults(func=cmd_add)

    g = sub.add_parser("generate", help="Packages/Release erzeugen, alte Nightlies entfernen")
    g.add_argument("--repo", required=True)
    g.add_argument("--dists", nargs="*", help="Distributionen, die immer angelegt werden")
    g.add_argument("--keep-nightly", type=int, default=5)
    g.add_argument("--gpg-key", default="", help="GPG-Key-ID zum Signieren (oder $APT_GPG_KEY_ID)")
    g.add_argument("--base-url", default="https://bene726.github.io/Fotobox_qt6")
    g.set_defaults(func=cmd_generate)

    args = ap.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
