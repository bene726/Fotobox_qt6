# Ein Programm je Testdatei. ctest startet sie einzeln, damit ein Absturz in
# einem Bereich die uebrigen Ergebnisse nicht mitnimmt.
#
# Alle Tests laufen ohne Bildschirm und ohne Hardware: sie binden fotobox_core,
# nicht das Programm. Was Kamera, GPIO oder Oberflaeche braucht, steht hier
# bewusst nicht - siehe TODO.md.

set(FOTOBOX_TESTS_LIST
    config
    filewatcher
    framegate
    storage
    theme
    qrencoder
    systemdwatchdog
    gphoto
)

# Einzelne Tests brauchen Quellen, die nicht in fotobox_core liegen, weil sie
# zur Hardware gehoeren. CameraGPhoto fasst beim blossen Uebersetzen keine
# Kamera an - geprueft wird hier nur das Auswerten der gphoto2-Ausgabe.
set(FOTOBOX_EXTRA_gphoto
    ${CMAKE_SOURCE_DIR}/src/hardware/Camera.cpp
    ${CMAKE_SOURCE_DIR}/src/hardware/CameraGPhoto.cpp)

foreach(name IN LISTS FOTOBOX_TESTS_LIST)
    add_executable(tst_${name} tst_${name}.cpp ${FOTOBOX_EXTRA_${name}})
    target_link_libraries(tst_${name} PRIVATE fotobox_core Qt6::Test)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # Im Testcode sind Warnungen immer Fehler: hier gibt es keinen
        # Altbestand, den man erst aufraeumen muesste.
        target_compile_options(tst_${name} PRIVATE -Wall -Wextra -Werror)
    endif()

    add_test(NAME ${name} COMMAND tst_${name})
    set_tests_properties(${name} PROPERTIES
        # Qt braucht eine Plattform; ueber SSH und in der CI gibt es keine Anzeige.
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        TIMEOUT 120)
endforeach()
