add_library(lgo.GUI.Vulkan.pipeline_state.def STATIC)
add_library(lgo.GUI.Vulkan.pipeline_state.impl STATIC)

target_sources(
    lgo.GUI.Vulkan.pipeline_state.def
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

target_sources(
    lgo.GUI.Vulkan.pipeline_state.impl
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.GUI.Vulkan.pipeline_state.def
    PRIVATE
        common
        external.Vulkan
        lgo.GUI.Vulkan.shader.def
        lgo.GUI.Vulkan.descriptor_set_layout.def
)

target_link_libraries(
    lgo.GUI.Vulkan.pipeline_state.impl
    PRIVATE
        common
        lgo.GUI.Vulkan.def
        external.Vulkan
)
