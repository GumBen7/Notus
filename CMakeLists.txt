cmake_minimum_required(VERSION 3.19)
project(Notus LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets LinguistTools Network)
find_library(QTKEYCHAIN_LIBRARY NAMES Qt6Keychain PATHS "C:/Qt/Tools/QtKeychain/lib")

qt_standard_project_setup()

qt_add_executable(Notus
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)

qt_add_translations(
    TARGETS Notus
    TS_FILES Notus_ru_RU.ts
)

target_link_libraries(Notus
    PRIVATE
        ${QTKEYCHAIN_LIBRARY}
        Qt::Core
        Qt::Widgets
        Qt::Network
        crypt32
)

include(GNUInstallDirs)
include_directories("C:/Qt/Tools/QtKeychain/include")

install(TARGETS Notus
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET Notus
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
