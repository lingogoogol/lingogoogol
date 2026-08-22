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
)

add_dependencies(
    lgo.GUI.engine
    lgo.GUI.engine.shader
)

function(add_slang_target TARGET)
    cmake_parse_arguments("SLANG" "" "" "SOURCES" ${ARGN})
    set(ENTRY_POINTS -entry vertex_main -entry fragment_main)

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shader.spv
        COMMAND
            slangc
            ${SLANG_SOURCES}
            -target spirv
            -profile spirv_1_4
            -emit-spirv-directly
            -fvk-use-entrypoint-name
            ${ENTRY_POINTS}
            -o ${CMAKE_CURRENT_BINARY_DIR}/slang.spv
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        VERBATIM
    )

    add_custom_target(
        ${TARGET}
        DEPENDS
            ${CMAKE_CURRENT_BINARY_DIR}/shader.spv
    )
endfunction()

add_slang_target(
    lgo.GUI.engine.shader
    SOURCES
        shader.slang
)
