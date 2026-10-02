add_library(lgo.GUI.Vulkan.descriptor_set_layout.def STATIC)
add_library(lgo.GUI.Vulkan.descriptor_set_layout.impl STATIC)

target_sources(
    lgo.GUI.Vulkan.descriptor_set_layout.def
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

target_sources(
    lgo.GUI.Vulkan.descriptor_set_layout.impl
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.GUI.Vulkan.descriptor_set_layout.def
    PRIVATE
        common
        external.Vulkan
)

target_link_libraries(
    lgo.GUI.Vulkan.descriptor_set_layout.impl
    PRIVATE
        common
        lgo.GUI.Vulkan.def
        external.Vulkan
)
