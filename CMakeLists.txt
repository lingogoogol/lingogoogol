cmake_minimum_required(VERSION 4.0...4.4 FATAL_ERROR)

# CMake's experimental `import std;` UUID is version-specific:
# - CMake 4.0..4.2 uses "d0edc3af-4c50-42ea-a356-e2862fe7a444"
# - CMake 4.3+     uses "f35a9ac6-8463-4d38-8eec-5d6008153e7d"
if(CMAKE_VERSION VERSION_LESS 4.3)
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
else()
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "f35a9ac6-8463-4d38-8eec-5d6008153e7d")
endif()
set(CMAKE_CXX_MODULE_STD ON)

# IMPORTANT: Keep `CMAKE_CXX_STANDARD` and `CMAKE_CXX_EXTENSIONS OFF` BEFORE `project()`.
# CMake synthesizes the `__cmake_cxx23` (`import std;`) target during `project(... LANGUAGES CXX)`.
# If `CMAKE_CXX_EXTENSIONS OFF` is placed after `project()`, Clang/GCC will compile `std.pcm` with
# `-std=gnu++23` (GNU extensions enabled) while project targets use `-std=c++23` (due to `-pedantic-errors`),
# causing Clang to reject importing `std.pcm` due to a language-extension flag mismatch.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(
    lingogoogol
    VERSION 0.0.5.0
    LANGUAGES CXX
)

set(CMAKE_COMPILE_WARNING_AS_ERROR OFF)
set(
    CMAKE_INTERPROCEDURAL_OPTIMIZATION
    $<$<CONFIG:Debug>:OFF>$
    $<$<CONFIG:Release>:ON>$
    $<$<CONFIG:RelWithDebInfo>:OFF>$
    $<$<CONFIG:MinSizeRel>:ON>$
)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(
    APPEND CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_LIST_DIR}/../CMake"
)

add_library(common INTERFACE)
set(
    COMMON_COMPILE_OPT_MSVC
    /permissive-
    /volatile:iso
    /utf-8
    /Zc:enumTypes
    /W4
    /options:strict
    /diagnostics:caret
    /EHsc
)
set(
    COMMON_COMPILE_OPT_GNU
    -Wall
    -Wextra
    --pedantic-errors
)
set(
    COMMON_COMPILE_OPT_CLANG
    -Wall
    -Wextra
    -pedantic-errors
)
target_compile_options(
    common
    INTERFACE
        $<$<CXX_COMPILER_ID:MSVC>:${COMMON_COMPILE_OPT_MSVC}>
        $<$<CXX_COMPILER_ID:GNU>:${COMMON_COMPILE_OPT_GNU}>
        $<$<CXX_COMPILER_ID:Clang>:${COMMON_COMPILE_OPT_CLANG}>
)
target_link_options(
    common
    INTERFACE
        $<$<CXX_COMPILER_ID:MSVC>:/NODEFAULTLIB:LIBCMT>
)

set(VCPKG_STATIC_INCLUDE_DIR "C:/lingogoogol/application/vcpkg/installed/x64-windows-static/include")
set(VCPKG_DYNAMIC_INCLUDE_DIR "C:/lingogoogol/application/vcpkg/installed/x64-windows/include/")
target_include_directories(
    common
    INTERFACE
        "../source/"
        "../include/"
        ${VCPKG_STATIC_INCLUDE_DIR}
        ${VCPKG_DYNAMIC_INCLUDE_DIR}
)

if(MSVC)
    target_link_directories(
        common
        INTERFACE
            "../lib/"
            "C:/lingogoogol/application/vcpkg/installed/x64-windows-static/lib/"
            "C:/lingogoogol/application/vcpkg/installed/x64-windows/lib/"
    )
endif()

add_subdirectory(external)
add_subdirectory(lgo)
add_subdirectory(storage)
add_subdirectory(other)
