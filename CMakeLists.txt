cmake_minimum_required(VERSION 3.16)

project(fotobox LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(FOTOBOX_PI "Build for Raspberry Pi (echte Hardware als Standard-Backends)" OFF)

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

if(FOTOBOX_PI)
    target_compile_definitions(fotobox PRIVATE FOTOBOX_PI)
else()
    target_compile_definitions(fotobox PRIVATE FOTOBOX_HOST)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(fotobox PRIVATE -Wall -Wextra)
endif()
