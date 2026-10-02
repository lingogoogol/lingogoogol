add_library(lgo.GUI.Vulkan.shader.def STATIC)
add_library(lgo.GUI.Vulkan.shader.impl STATIC)

target_sources(
    lgo.GUI.Vulkan.shader.def
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

target_sources(
    lgo.GUI.Vulkan.shader.impl
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.GUI.Vulkan.shader.def
    PRIVATE
        common
        external.Vulkan
        lgo.GUI.Vulkan.shader_module.def
)

target_link_libraries(
    lgo.GUI.Vulkan.shader.impl
    PRIVATE
        common
        lgo.GUI.Vulkan.def
        external.Vulkan
)
