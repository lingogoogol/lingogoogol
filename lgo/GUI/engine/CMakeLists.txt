add_library(lgo.GUI.engine OBJECT)

target_sources(
    lgo.GUI.engine
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.GUI.engine
    PRIVATE
        common
        external.Vulkan
        external.GLFW
        lgo.math.vec
        lgo.io.file
)

target_compile_definitions(
    lgo.GUI.engine
    PRIVATE
        LGO_SHADER_PATH="${CMAKE_CURRENT_BINARY_DIR}/shader.spv"
)

function(add_slang_target TARGET)
    cmake_parse_arguments("SLANG" "" "" "SOURCES" ${ARGN})
    set(ENTRY_POINTS -entry vertex_main -entry fragment_main)

    find_program(SLANG_COMPILER NAMES slangc REQUIRED)

    set(SPIRV_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/shader.spv")

    add_custom_command(
        OUTPUT "${SPIRV_OUTPUT}"
        COMMAND
            "${SLANG_COMPILER}"
            ${SLANG_SOURCES}
            -target spirv
            -profile spirv_1_4
            -emit-spirv-directly
            -fvk-use-entrypoint-name
            ${ENTRY_POINTS}
            -o "${SPIRV_OUTPUT}"
        DEPENDS
            ${SLANG_SOURCES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        VERBATIM
    )

    add_custom_target(
        ${TARGET}
        DEPENDS
            "${SPIRV_OUTPUT}"
    )
endfunction()

add_slang_target(
    lgo.GUI.engine.shader
    SOURCES
        shader.slang
)

add_dependencies(
    lgo.GUI.engine
    lgo.GUI.engine.shader
)
