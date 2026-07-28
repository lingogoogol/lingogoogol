cmake_minimum_required(VERSION 4.0...4.4)

project(
    lingogoogol
    VERSION 0.0.5.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(common INTERFACE)
target_compile_options(
    common
    INTERFACE
)
target_include_directories(
    common
    INTERFACE
        "../source/"
        "../include/"
        "C:/lingogoogol/application/vcpkg/installed/x64-windows/include/"
)
target_link_directories(
    common
    INTERFACE
        "../lib/"
        "C:/lingogoogol/application/vcpkg/installed/x64-windows/lib/"
)

add_subdirectory(storage)
