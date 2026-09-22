#!/usr/bin/env python3
"""Erzeugt den Text für ein GitHub-Release oder den Nightly-Eintrag.

Enthält die Änderungen seit dem letzten Release-Tag, die Installationsbefehle
für den Raspberry Pi und eine Tabelle der angehängten Pakete.

Beispiel:
  scripts/release_notes.py --version 0.1.0 --channel main --tag v0.1.0 \
      --repo bene726/Fotobox_qt6 out/*.deb > notes.md
"""
import argparse
import hashlib
import re
import subprocess
import sys
from pathlib import Path

DIST_NAMES = {"bookworm": ("Bookworm", "12"), "trixie": ("Trixie", "13")}


def git(*args, default=None):
    try:
        r = subprocess.run(["git", *args], capture_output=True, text=True, check=True)
        return r.stdout.strip()
    except subprocess.CalledProcessError:
        return default


def previous_tag(tag: str | None) -> str | None:
    """Letzter Release-Tag vor 'tag' (bzw. vor HEAD)."""
    ref = f"{tag}^" if tag else "HEAD"
    return git("describe", "--tags", "--abbrev=0", "--match", "v*", ref, default=None)


def commits(prev: str | None, ref: str) -> list:
    rng = f"{prev}..{ref}" if prev else ref
    out = git("log", "--no-merges", "--pretty=%h\t%s", rng, default="")
    rows = []
    for line in out.splitlines():
        if "\t" not in line:
            continue
        sha, subject = line.split("\t", 1)
        rows.append((sha, subject))
    return rows


def dist_of(name: str) -> str | None:
    m = re.search(r"_(bookworm|trixie)\.deb$", name)
    return m.group(1) if m else None


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--version", required=True)
    ap.add_argument("--channel", required=True, choices=["main", "nightly"])
    ap.add_argument("--tag", default="")
    ap.add_argument("--repo", required=True, help="owner/name")
    ap.add_argument("--base-url", default="")
    ap.add_argument("--ref", default="HEAD")
    ap.add_argument("debs", nargs="*")
    a = ap.parse_args()

    base_url = a.base_url or f"https://{a.repo.split('/')[0]}.github.io/{a.repo.split('/')[1]}"
    is_release = a.channel == "main"
    prev = previous_tag(a.tag if is_release else None)
    rows = commits(prev, a.tag if is_release and a.tag else a.ref)

    out = []
    if not is_release:
        out += [
            f"Automatischer Build vom Stand `{git('rev-parse', '--short', 'HEAD', default='?')}` "
            f"auf `main`.",
            "",
            "> Nightlies sind ungetestete Zwischenstände. Für eine echte Feier bitte ein Release verwenden.",
            "",
        ]

    out.append(f"## Änderungen{f' seit {prev}' if prev else ''}")
    out.append("")
    if rows:
        MAX = 25
        for sha, subject in rows[:MAX]:
            out.append(f"- {subject} ([`{sha}`](https://github.com/{a.repo}/commit/{sha}))")
        if len(rows) > MAX:
            out.append(f"- … und {len(rows) - MAX} weitere Commits")
    else:
        out.append("- keine Commits gefunden")
    out.append("")
    if prev and a.tag:
        out.append(f"[Vollständiger Vergleich](https://github.com/{a.repo}/compare/{prev}...{a.tag})")
        out.append("")

    # --- Installation
    component = "main" if is_release else "main nightly"
    out += [
        "## Installation auf dem Raspberry Pi (64-bit)",
        "",
        "```bash",
        "sudo mkdir -p /etc/apt/keyrings",
        f"curl -fsSL {base_url}/fotobox.gpg | sudo tee /etc/apt/keyrings/fotobox.gpg >/dev/null",
        ". /etc/os-release",
        f'echo "deb [arch=arm64 signed-by=/etc/apt/keyrings/fotobox.gpg] {base_url} '
        f'$VERSION_CODENAME {component}" | sudo tee /etc/apt/sources.list.d/fotobox.list',
        "sudo apt update && sudo apt install fotobox",
        "```",
        "",
    ]
    if not is_release:
        out += ["Die Komponente `nightly` liefert genau diesen Stand.", ""]
    out += ["Spätere Updates kommen über `sudo apt upgrade`.", ""]

    # --- Pakete
    files = [Path(p) for p in a.debs if Path(p).is_file()]
    if files:
        out += [
            "## Pakete",
            "",
            "| Datei | Raspberry Pi OS | Größe |",
            "|---|---|---|",
        ]
        for f in sorted(files):
            d = dist_of(f.name)
            label = f"{DIST_NAMES[d][0]} ({DIST_NAMES[d][1]})" if d in DIST_NAMES else "unbekannt"
            out.append(f"| `{f.name}` | {label} | {f.stat().st_size // 1024} KB |")
        out += [
            "",
            "Beide Pakete sind `arm64` und laufen auf Raspberry Pi 4 und 5.",
            "Die Prüfsummen stehen in `SHA256SUMS`.",
            "",
            "<details><summary>SHA256</summary>",
            "",
            "```",
        ]
        for f in sorted(files):
            out.append(f"{hashlib.sha256(f.read_bytes()).hexdigest()}  {f.name}")
        out += ["```", "", "</details>", ""]

    sys.stdout.write("\n".join(out).rstrip() + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
