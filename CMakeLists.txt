cmake_minimum_required(VERSION 3.28)

project(QBitMPlayer
    VERSION 2.7.0
    LANGUAGES CXX
    DESCRIPTION "Music Player written in C++ & Qt."
)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS LinguistTools Multimedia Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS LinguistTools Multimedia Widgets)

set(TS_FILES resources/i18n/QBitMPlayer_es_MX.ts)
set(LICENSE "MIT")
set(DESKTOP_FILE resources/qbitmplayer.desktop)

set(PROJECT_SOURCES
    config.hpp.in
    main.cpp
    mainwindow.cpp
    mainwindow.hpp
    mainwindow.ui
    player.hpp
    player.cpp
    playlistchooser.hpp
    playlistchooser.cpp
    playlistchooser.ui
    settings.hpp
    settings.cpp
    settings.ui
    ../${TS_FILES}
    ../resources.qrc
    ../resources/qbitmplayer.desktop
)

list(TRANSFORM PROJECT_SOURCES PREPEND "src/")

string(REPLACE "/" "\\/" INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
execute_process(
    COMMAND sed -i "s/Exec=.*/Exec=${INSTALL_PREFIX}\\/bin\\/qbitmplayer/g" ${DESKTOP_FILE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
execute_process(
    COMMAND sed -i "s/Icon=.*/Icon=${INSTALL_PREFIX}\\/share\\/${PROJECT_NAME}\\/resources\\/assets\\/icon.png/g" ${DESKTOP_FILE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S")
configure_file(src/config.hpp.in config.hpp)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

    qt_add_executable(qbitmplayer
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${QM_FILES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET QBitMPlayer APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

else()
    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

    if(ANDROID)
        add_library(qbitmplayer SHARED
            ${PROJECT_SOURCES}
            ${QM_FILES}
        )
    # Define properties for Android with Qt 5 after find_package() calls as:
    #    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(qbitmplayer
            ${PROJECT_SOURCES}
            ${QM_FILES}
        )
    endif()
endif()

target_link_libraries(qbitmplayer PRIVATE
    Qt${QT_VERSION_MAJOR}::Multimedia
    Qt${QT_VERSION_MAJOR}::Widgets
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.salazarcodes.QBitMPlayer)
endif()
set_target_properties(qbitmplayer PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS qbitmplayer
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES resources/qbitmplayer.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
install(FILES ${QM_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/resources/i18n)
install(DIRECTORY resources/assets DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/resources)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(qbitmplayer)
endif()
