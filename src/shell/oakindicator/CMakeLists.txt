# OAK 状态指示器 — Qt6 + wlroots 胶水桥 (CMake, C++17)
cmake_minimum_required(VERSION 3.16)
project(openos-oak-indicator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)          # 兼容 CMAKE_CXX_STANDARD 17
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Qml)
find_package(PkgConfig REQUIRED)
pkg_check_modules(WLROOTS REQUIRED wlroots)
pkg_check_modules(WAYLAND_SERVER REQUIRED wayland-server)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# QML 资源
qt6_add_resources(OAK_QML qml_resources.qrc)

add_executable(openos-oak-indicator
    main.cpp
    oak_bridge.cpp
    oak_mainloop.cpp
    ${OAK_QML}
)

target_include_directories(openos-oak-indicator PRIVATE
    ${WLROOTS_INCLUDE_DIRS}
    ${WAYLAND_SERVER_INCLUDE_DIRS}
)

target_link_libraries(openos-oak-indicator PRIVATE
    Qt6::Core Qt6::Gui Qt6::Quick Qt6::Qml
    ${WLROOTS_LIBRARIES}
    ${WAYLAND_SERVER_LIBRARIES}
)

target_compile_definitions(openos-oak-indicator PRIVATE
    ${WLROOTS_CFLAGS_OTHER}
)
