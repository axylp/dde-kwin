set(TARGET_NAME "deepin-chameleon")

add_definitions(-DTARGET_NAME=\\"${TARGET_NAME}\\")

find_package(Qt5 CONFIG REQUIRED COMPONENTS Core)
find_package(KF5CoreAddons REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(KF5Config REQUIRED)
find_package(KF5WindowSystem REQUIRED)
find_package(KDecoration2 REQUIRED)
find_package(Qt5LinguistTools REQUIRED)
include_directories(${Qt5Core_PRIVATE_INCLUDE_DIRS})
include_directories(${Qt5Gui_PRIVATE_INCLUDE_DIRS})

option(DISABLE_LIB_KWIN_XCB "Don't use libkwin-xcb" OFF)

set(CMAKE_CXX_STANDARD 11)

# translations
file(GLOB TS LIST_DIRECTORIES false translations/splitmenu*.ts)
foreach(_ts_file ${TS})
    execute_process(
        COMMAND ${LUPDATE_EXECUTABLE} -recursive ${CMAKE_SOURCE_DIR}/translations -ts ${_ts_file})
    execute_process(COMMAND lrelease ${_ts_file})
endforeach()

add_library(
    ${TARGET_NAME}
    MODULE
    main.cpp
    chameleon.cpp
    chameleontheme.cpp
    chameleonbutton.cpp
    chameleonshadow.cpp
    chameleonconfig.cpp
    chameleonwindowtheme.cpp
    chameleonsplitmenu.cpp
    themes.qrc
)

target_link_libraries(
    ${TARGET_NAME}
    PUBLIC
    Qt5::Core
    Qt5::DBus
    PRIVATE
    KDecoration2::KDecoration
    KF5::CoreAddons
    KF5::ConfigCore
    KF5::WindowSystem
)

if (DISABLE_LIB_KWIN_XCB)
    add_definitions(-DDISBLE_DDE_KWIN_XCB)
else()
    include_directories(../platforms/lib)
    target_link_libraries(${TARGET_NAME} kwin-xcb)
endif()

install(TARGETS ${TARGET_NAME} DESTINATION ${QT_INSTALL_PLUGINS}/org.kde.kdecoration2)
