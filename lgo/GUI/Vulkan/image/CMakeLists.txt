add_library(lgo.GUI.Vulkan.image.def STATIC)
add_library(lgo.GUI.Vulkan.image.impl STATIC)

target_sources(
    lgo.GUI.Vulkan.image.def
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

target_sources(
    lgo.GUI.Vulkan.image.impl
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.GUI.Vulkan.image.def
    PRIVATE
        common
        external.Vulkan
        lgo.GUI.texture
        lgo.GUI.Vulkan.buffer.def
        lgo.GUI.Vulkan.memory.def
)

target_link_libraries(
    lgo.GUI.Vulkan.image.impl
    PRIVATE
        common
        lgo.GUI.Vulkan.def
        external.Vulkan
        lgo.GUI.texture
)
