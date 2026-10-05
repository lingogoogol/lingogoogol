add_library(lgo.GUI.primitive.engine.decl STATIC)
add_library(lgo.GUI.primitive.engine.def STATIC)
add_library(lgo.GUI.primitive.engine.impl STATIC)

target_sources(
    lgo.GUI.primitive.engine.decl
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            decl.cppm
)

target_sources(
    lgo.GUI.primitive.engine.def
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

target_sources(
    lgo.GUI.primitive.engine.impl
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.GUI.primitive.engine.decl
    PRIVATE
        common
)

target_link_libraries(
    lgo.GUI.primitive.engine.def
    PRIVATE
        common
        external.Vulkan
        external.GLFW
        lgo.math.vec
        lgo.io.file
        lgo.GUI.Vulkan
        lgo.GUI.primitive.rect.def
)

target_link_libraries(
    lgo.GUI.primitive.engine.impl
    PRIVATE
        common
        lgo.GUI.primitive.def
        external.Vulkan
        external.GLFW
        lgo.math.vec
        lgo.io.file
        lgo.GUI.Vulkan
)

add_dependencies(
    lgo.GUI.primitive.engine.def
    lgo.GUI.primitive.engine.shader_solid
    lgo.GUI.primitive.engine.shader_texture
)

function(add_slang_target TARGET OUT_NAME)
    cmake_parse_arguments("SLANG" "" "" "SOURCES" ${ARGN})
    set(ENTRY_POINTS -entry vertex_main -entry fragment_main)

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${OUT_NAME}.spv
        COMMAND
            slangc
            ${SLANG_SOURCES}
            -target spirv
            -profile spirv_1_4
            -emit-spirv-directly
            -fvk-use-entrypoint-name
            ${ENTRY_POINTS}
            -o ${CMAKE_CURRENT_BINARY_DIR}/${OUT_NAME}.spv
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        DEPENDS ${SLANG_SOURCES}
        VERBATIM
    )

    add_custom_target(
        ${TARGET}
        DEPENDS
            ${CMAKE_CURRENT_BINARY_DIR}/${OUT_NAME}.spv
    )

    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/${OUT_NAME}.spv
        DESTINATION
            ${CMAKE_INSTALL_PREFIX}/shader
    )
endfunction()

add_slang_target(
    lgo.GUI.primitive.engine.shader_solid
    solid
    SOURCES
        solid.slang
)

add_slang_target(
    lgo.GUI.primitive.engine.shader_texture
    texture
    SOURCES
        texture.slang
)
