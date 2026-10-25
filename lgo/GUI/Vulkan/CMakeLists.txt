add_library(lgo.GUI.Vulkan INTERFACE)
add_library(lgo.GUI.Vulkan.def STATIC)

target_sources(
    lgo.GUI.Vulkan.def
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

add_subdirectory(debug)
add_subdirectory(support)
add_subdirectory(pipeline_state)
add_subdirectory(descriptor_set_layout)
add_subdirectory(shader_module)
add_subdirectory(shader)
add_subdirectory(memory)
add_subdirectory(buffer)
add_subdirectory(image)
add_subdirectory(image_view)

target_link_libraries(
    lgo.GUI.Vulkan.def
    PRIVATE
        common
        lgo.GUI.Vulkan.debug.def
        lgo.GUI.Vulkan.support.def
        lgo.GUI.Vulkan.pipeline_state.def
        lgo.GUI.Vulkan.descriptor_set_layout.def
        lgo.GUI.Vulkan.shader_module.def
        lgo.GUI.Vulkan.shader.def
        lgo.GUI.Vulkan.memory.def
        lgo.GUI.Vulkan.buffer.def
        lgo.GUI.Vulkan.image.def
        lgo.GUI.Vulkan.image_view.def
)

target_link_libraries(
    lgo.GUI.Vulkan
    INTERFACE
        lgo.GUI.Vulkan.def
        lgo.GUI.Vulkan.pipeline_state.impl
        lgo.GUI.Vulkan.descriptor_set_layout.impl
        lgo.GUI.Vulkan.shader_module.impl
        lgo.GUI.Vulkan.shader.impl
        lgo.GUI.Vulkan.memory.impl
        lgo.GUI.Vulkan.buffer.impl
        lgo.GUI.Vulkan.image.impl
        lgo.GUI.Vulkan.image_view.impl
)
