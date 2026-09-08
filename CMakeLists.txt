cmake_minimum_required(VERSION 4.0...4.4 FATAL_ERROR)

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "f35a9ac6-8463-4d38-8eec-5d6008153e7d")
set(CMAKE_CXX_MODULE_STD ON)

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
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
        $<$<CXX_COMPILER_ID:Clang>:-Wall>
        $<$<CXX_COMPILER_ID:Clang>:-Wextra>
        $<$<CXX_COMPILER_ID:Clang>:-pedantic-errors>
)
add_subdirectory(external)
add_subdirectory(lgo)

option(LINGOGOOGOL_BUILD_STORAGE "Build the legacy storage GUI while it is being ported to Vulkan" OFF)
if(LINGOGOOGOL_BUILD_STORAGE)
    add_subdirectory(storage)
endif()

include(CTest)
if(BUILD_TESTING)
    add_test(
        NAME source-regressions
        COMMAND
            powershell
            -NoProfile
            -ExecutionPolicy Bypass
            -File "${CMAKE_CURRENT_SOURCE_DIR}/tests/regression.ps1"
    )
endif()
