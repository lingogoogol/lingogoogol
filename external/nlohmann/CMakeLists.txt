add_library(external.nlohmann STATIC)

set(NLOHMANN_DIR ${VCPKG_DYNAMIC_INCLUDE_DIR})

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/nlohmann.ifc
        COMMAND
            cl
            /exportHeader /headerName:angle "${NLOHMANN_DIR}/nlohmann/json.hpp"
            /ifcOutput ${CMAKE_CURRENT_BINARY_DIR}/nlohmann.ifc
            ${COMMON_COMPILE_OPT_MSVC}
            /MDd
            /c
            /std:c++latest
            /I${VCPKG_DYNAMIC_INCLUDE_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        VERBATIM
    )
    add_custom_target(
        external.nlohmann.header_unit
        DEPENDS
            ${CMAKE_CURRENT_BINARY_DIR}/nlohmann.ifc
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/nlohmann.gcm
        COMMAND
            g++
            -fmodules -x c++-system-header "${NLOHMANN_DIR}/nlohmann/json.hpp"
            -o ${CMAKE_CURRENT_BINARY_DIR}/nlohmann.gcm
            ${COMMON_COMPILE_OPT_GNU}
            -c
            -std=c++23
            -I${VCPKG_DYNAMIC_INCLUDE_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        VERBATIM
    )
    add_custom_target(
        external.nlohmann.header_unit
        DEPENDS
            ${CMAKE_CURRENT_BINARY_DIR}/nlohmann.gcm
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/nlohmann.pcm
        COMMAND
            clang
            -fmodule-header "${NLOHMANN_DIR}/nlohmann/json.hpp"
            -o ${CMAKE_CURRENT_BINARY_DIR}/nlohmann.pcm
            ${COMMON_COMPILE_OPT_CLANG}
            -std=c++23
            -I${VCPKG_DYNAMIC_INCLUDE_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        VERBATIM
    )
    add_custom_target(
        external.nlohmann.header_unit
        DEPENDS
            ${CMAKE_CURRENT_BINARY_DIR}/nlohmann.pcm
    )
endif()

add_dependencies(
    external.nlohmann
    external.nlohmann.header_unit
)

#Add the compile options to a separate target
#because otherwise it will not be included during dependency scanning of the module.
add_library(external.nlohmann.compile_options INTERFACE)
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(
        external.nlohmann.compile_options
        INTERFACE
            /headerUnit ${NLOHMANN_DIR}/nlohmann/json.hpp=${CMAKE_CURRENT_BINARY_DIR}/nlohmann.ifc
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(
        external.nlohmann.compile_options
        INTERFACE
            -fmodule-file=${CMAKE_CURRENT_BINARY_DIR}/nlohmann.pcm
    )
endif()

target_sources(
    external.nlohmann
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

target_link_libraries(
    external.nlohmann
    PRIVATE
        common
    PUBLIC
        external.nlohmann.compile_options
)
