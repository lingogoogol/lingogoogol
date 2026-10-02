add_library(lgo.GUI.Vulkan.image_view.def STATIC)
add_library(lgo.GUI.Vulkan.image_view.impl STATIC)

target_sources(
    lgo.GUI.Vulkan.image_view.def
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

target_sources(
    lgo.GUI.Vulkan.image_view.impl
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.GUI.Vulkan.image_view.def
    PRIVATE
        common
        external.Vulkan
        lgo.GUI.Vulkan.image.def
)

target_link_libraries(
    lgo.GUI.Vulkan.image_view.impl
    PRIVATE
        common
        lgo.GUI.Vulkan.def
        external.Vulkan
)
