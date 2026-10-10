cmake_minimum_required(VERSION 3.16)

project(hex-calc
    VERSION 1.0.0
    DESCRIPTION "A themeable programmer's calculator for Linux"
    HOMEPAGE_URL "https://github.com/naved124/hex-calc"
    LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(HEXCALC_WERROR "Treat compiler warnings as errors" OFF)
option(HEXCALC_BUILD_TESTS "Build the engine unit tests" ON)

# Qt 6 preferred, Qt 5.12+ works too. -DHEXCALC_QT=5 forces Qt 5.
set(HEXCALC_QT "" CACHE STRING "Qt major version to use (5 or 6); empty picks the newest")
if(HEXCALC_QT)
    set(QT_VERSION_MAJOR ${HEXCALC_QT})
else()
    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
endif()
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
message(STATUS "Building with Qt ${Qt${QT_VERSION_MAJOR}Widgets_VERSION}")

include(GNUInstallDirs)
set(HEXCALC_APP_ID "io.github.naved124.HexCalc")
set(HEXCALC_DATA_INSTALL_DIR "${CMAKE_INSTALL_DATADIR}/hex-calc")

set(WARNINGS -Wall -Wextra)
if(HEXCALC_WERROR)
    list(APPEND WARNINGS -Werror)
endif()

# Calculator core: plain C, no GUI dependencies.
add_library(hexcalc_engine STATIC src/engine.c)
target_include_directories(hexcalc_engine PUBLIC src)
target_compile_options(hexcalc_engine PRIVATE ${WARNINGS})

add_executable(hex-calc
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/preferences.cpp
    src/preferences.h
    src/themes.cpp
    src/themes.h
    src/settings.cpp
    src/settings.h
    src/history.cpp
    src/history.h
    src/icons.cpp
    src/icons.h
)
target_compile_definitions(hex-calc PRIVATE
    HEXCALC_VERSION="${PROJECT_VERSION}"
    HEXCALC_APP_ID="${HEXCALC_APP_ID}"
    HEXCALC_DATADIR="${CMAKE_INSTALL_FULL_DATADIR}/hex-calc"
)
target_compile_options(hex-calc PRIVATE ${WARNINGS})
target_link_libraries(hex-calc PRIVATE hexcalc_engine Qt${QT_VERSION_MAJOR}::Widgets)

if(HEXCALC_BUILD_TESTS)
    enable_testing()
    add_executable(test_engine tests/test_engine.c)
    target_link_libraries(test_engine PRIVATE hexcalc_engine)
    target_compile_options(test_engine PRIVATE ${WARNINGS})
    add_test(NAME engine COMMAND test_engine)
endif()

# Installation ------------------------------------------------------------

install(TARGETS hex-calc RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY assets/fonts DESTINATION ${HEXCALC_DATA_INSTALL_DIR})
install(FILES assets/hex-calc.svg assets/hex-calc.png DESTINATION ${HEXCALC_DATA_INSTALL_DIR})
install(FILES assets/hex-calc.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
install(FILES assets/hex-calc.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps)

set(HEXCALC_EXEC "${CMAKE_INSTALL_FULL_BINDIR}/hex-calc")
configure_file(assets/${HEXCALC_APP_ID}.desktop.in ${HEXCALC_APP_ID}.desktop @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${HEXCALC_APP_ID}.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
