add_library(lgo.GUI.Vulkan.shader_module.def STATIC)
add_library(lgo.GUI.Vulkan.shader_module.impl STATIC)

target_sources(
    lgo.GUI.Vulkan.shader_module.def
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

target_sources(
    lgo.GUI.Vulkan.shader_module.impl
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.GUI.Vulkan.shader_module.def
    PRIVATE
        common
        external.Vulkan
        lgo.io.file
)

target_link_libraries(
    lgo.GUI.Vulkan.shader_module.impl
    PRIVATE
        common
        lgo.GUI.Vulkan.def
        external.Vulkan
        lgo.io.file
)
