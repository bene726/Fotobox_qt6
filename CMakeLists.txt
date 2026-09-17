cmake_minimum_required(VERSION 3.16)

project(fotobox LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(FOTOBOX_PI "Build for Raspberry Pi (echte Hardware als Standard-Backends)" OFF)
set(FOTOBOX_VERSION "0.0.0" CACHE STRING "Versionsstring fuer Programm und Paket")
set(FOTOBOX_MAINTAINER "Bene <46480461+BenediktCyranka@users.noreply.github.com>"
    CACHE STRING "Maintainer-Feld des Debian-Pakets")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt6 OPTIONAL_COMPONENTS Multimedia)

qt_add_executable(fotobox
    src/main.cpp
    src/MainWindow.cpp

    src/core/Config.cpp
    src/core/FileWatcher.cpp

    src/hardware/HardwareFactory.cpp
    src/hardware/Camera.cpp
    src/hardware/Trigger.cpp
    src/hardware/CameraDummy.cpp
    src/hardware/CameraGPhoto.cpp
    src/hardware/CameraSnapshot.cpp
    src/hardware/LedController.cpp
    src/hardware/LedPwm.cpp
    src/hardware/LiveViewCamera.cpp
    src/hardware/LiveViewCameraDummy.cpp
    src/hardware/LiveViewCameraRpicam.cpp
    src/hardware/TriggerKeyboard.cpp
    src/hardware/TriggerGpio.cpp
    src/hardware/TriggerTimer.cpp

    src/input/TouchInput.cpp
    src/input/TouchInputQt.cpp

    src/ui/QrCodeWidget.cpp
    src/ui/SettingsPanel.cpp
    src/ui/Theme.cpp
    src/ui/QrCard.cpp
    src/ui/qrcodegen.cpp
)

target_include_directories(fotobox PRIVATE src)
target_link_libraries(fotobox PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)

# Webcam-Livebild (Host) nur, wenn Qt Multimedia vorhanden ist.
if(Qt6Multimedia_FOUND)
    message(STATUS "Qt Multimedia gefunden: Webcam-Livebild aktiviert")
    target_sources(fotobox PRIVATE src/hardware/LiveViewCameraQt.cpp)
    target_link_libraries(fotobox PRIVATE Qt6::Multimedia)
    target_compile_definitions(fotobox PRIVATE FOTOBOX_HAS_QTMULTIMEDIA)
endif()

# macOS: Kamera-Berechtigung. Die Info.plist wird in das Binary eingebettet
# (kein App-Bundle noetig). Die Property sorgt dafuer, dass Qt beim Finalisieren
# das statische Permission-Plugin einlinkt.
if(APPLE)
    set(FOTOBOX_PLIST "${CMAKE_SOURCE_DIR}/macos/Info.plist")
    set_target_properties(fotobox PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${FOTOBOX_PLIST}")
    target_link_options(fotobox PRIVATE
        "LINKER:-sectcreate,__TEXT,__info_plist,${FOTOBOX_PLIST}")
endif()

target_compile_definitions(fotobox PRIVATE FOTOBOX_VERSION_STR="${FOTOBOX_VERSION}")

if(FOTOBOX_PI)
    target_compile_definitions(fotobox PRIVATE FOTOBOX_PI)
else()
    target_compile_definitions(fotobox PRIVATE FOTOBOX_HOST)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(fotobox PRIVATE -Wall -Wextra)
endif()

# ---------------------------------------------------------------- Installation

include(GNUInstallDirs)
install(TARGETS fotobox RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES packaging/fotobox.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES Config.example.json README.md DESTINATION ${CMAKE_INSTALL_DOCDIR})
# Optionale Schriften fuer die Designs (Ordner fonts/ im Quellbaum)
install(DIRECTORY fonts/ DESTINATION ${CMAKE_INSTALL_DATADIR}/fotobox/fonts OPTIONAL)

# ---------------------------------------------------------------- Debian-Paket (cpack -G DEB)

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "fotobox")
set(CPACK_PACKAGE_VERSION "${FOTOBOX_VERSION}")
set(CPACK_PACKAGE_CONTACT "${FOTOBOX_MAINTAINER}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Fotobox: Livebild, DSLR-Ausloesung, Slideshow und QR-Code")
set(CPACK_PACKAGE_DESCRIPTION
    "Qt6-Fotobox fuer Raspberry Pi mit Pi-Kamera als Livebild und DSLR ueber gphoto2.")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_STRIP_FILES ON)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${FOTOBOX_MAINTAINER}")
set(CPACK_DEBIAN_PACKAGE_SECTION "graphics")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/bene726/Fotobox_qt6")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)          # Bibliotheks-Abhaengigkeiten automatisch
set(CPACK_DEBIAN_PACKAGE_DEPENDS "gphoto2")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "rpicam-apps, gpiod")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)         # fotobox_<version>_<arch>.deb
set(CPACK_DEBIAN_COMPRESSION_TYPE "xz")
include(CPack)
