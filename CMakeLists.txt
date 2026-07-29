cmake_minimum_required(VERSION 4.0...4.4)

project(
    lingogoogol
    VERSION 0.0.5.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_COMPILE_WARNING_AS_ERROR OFF)
set(
    CMAKE_INTERPROCEDURAL_OPTIMIZATION
    $<$<CONFIG:Debug>:OFF>$
    $<$<CONFIG:Release>:ON>$
    $<$<CONFIG:RelWithDebInfo>:OFF>$
    $<$<CONFIG:MinSizeRel>:ON>$
)

add_library(common INTERFACE)
target_compile_options(
    common
    INTERFACE
        $<$<CXX_COMPILER_ID:MSVC>:/permissive->
        $<$<CXX_COMPILER_ID:MSVC>:/volatile:iso>
        $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
        $<$<CXX_COMPILER_ID:MSVC>:/Zc:enumTypes>
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
        $<$<CXX_COMPILER_ID:MSVC>:/options:strict>
        $<$<CXX_COMPILER_ID:MSVC>:/diagnostics:caret>
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
