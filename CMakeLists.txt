cmake_minimum_required(VERSION 3.28)

project(QBitMPlayer
    VERSION 3.1.0
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

if (ENABLE_IPC)
    find_package(QT NAMES Qt6 REQUIRED COMPONENTS DBus)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED DBus)

    add_compile_definitions(ENABLE_IPC="1")
endif()

if (ENABLE_NOTIFICATIONS)
    if (WIN32)
        include(ExternalProject)
        ExternalProject_Add(
            WinToast
            GIT_REPOSITORY "https://github.com/mohabouje/WinToast"
            GIT_TAG "master"
            SOURCE_DIR "${CMAKE_BINARY_DIR}/WinToast-src"
            BINARY_DIR "${CMAKE_BINARY_DIR}/WinToast-build"
            CMAKE_ARGS
                -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/WinToast-install
            UPDATE_COMMAND ""
            CONFIGURE_COMMAND cmake -G Ninja -S ${CMAKE_BINARY_DIR}/WinToast-src -B ${CMAKE_BINARY_DIR}/WinToast-build -DCMAKE_BUILD_TYPE=Release
            BUILD_COMMAND cmake --build ${CMAKE_BINARY_DIR}/WinToast-build
            INSTALL_COMMAND ""
        )

        include_directories(${CMAKE_BINARY_DIR}/WinToast-src/include)
    else()
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(libs REQUIRED IMPORTED_TARGET libnotify)
    endif()

    add_compile_definitions(ENABLE_NOTIFICATIONS="1")
endif()

if (ENABLE_VIDEO_PLAYER)
    find_package(QT NAMES Qt6 REQUIRED COMPONENTS MultimediaWidgets)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS MultimediaWidgets)

    add_compile_definitions(ENABLE_VIDEO_PLAYER="1")
endif()

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

if (ENABLE_NOTIFICATIONS)
    set(PROJECT_SOURCES
        ${PROJECT_SOURCES}
        notifier.hpp
        notifier.cpp
    )
endif()

if (ENABLE_VIDEO_PLAYER)
    set(PROJECT_SOURCES
        ${PROJECT_SOURCES}
        videoplayer.hpp
        videoplayer.cpp
    )
endif()

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

if (ENABLE_IPC)
    target_link_libraries(qbitmplayer PRIVATE
        Qt${QT_VERSION_MAJOR}::DBus
    )
endif()

if (ENABLE_NOTIFICATIONS)
    if (NOT WIN32)
        target_link_libraries(qbitmplayer PRIVATE
            PkgConfig::libs
        )
    else()
        add_dependencies(qbitmplayer WinToast)
        target_link_libraries(qbitmplayer PRIVATE ${CMAKE_BINARY_DIR}/WinToast-build/WinToast.lib)
        set_target_properties(qbitmplayer PROPERTIES WIN32_EXECUTABLE ON)
    endif()
endif()

if (ENABLE_VIDEO_PLAYER)
    target_link_libraries(qbitmplayer PRIVATE
        Qt${QT_VERSION_MAJOR}::MultimediaWidgets
    )
endif()

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
install(DIRECTORY resources/assets DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/resources)
install(FILES ${QM_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/resources/i18n)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(qbitmplayer)
endif()
