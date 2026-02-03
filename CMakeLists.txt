cmake_minimum_required(VERSION 3.16)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

project(fotobox LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(FOTOBOX_TARGET_PI "Build for Raspberry Pi" OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)

if(FOTOBOX_TARGET_PI)
    add_compile_definitions(FOTOBOX_PI)
else()
    add_compile_definitions(FOTOBOX_HOST)
endif()

set(CMAKE_AUTOMOC ON)

add_executable(fotobox
    src/main.cpp
    src/MainWindow.cpp

    src/core/Config.cpp
    src/core/FileWatcher.cpp

    src/hardware/LiveViewCamera.cpp
    src/hardware/LiveViewCameraDummy.cpp

    src/hardware/CameraFactory.cpp
    src/hardware/CameraDummy.cpp
    src/hardware/CameraGPhoto.cpp

    src/hardware/TriggerThread.cpp

    src/input/TouchInput.cpp
    src/input/TouchInputQt.cpp

    src/ui/QrCodeWidget.cpp
    src/ui/qrcodegen.cpp
)




target_include_directories(fotobox PRIVATE src)

target_link_libraries(fotobox
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
)
