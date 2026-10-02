add_library(lgo.GUI.Vulkan.buffer.def STATIC)
add_library(lgo.GUI.Vulkan.buffer.impl STATIC)

target_sources(
    lgo.GUI.Vulkan.buffer.def
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

target_sources(
    lgo.GUI.Vulkan.buffer.impl
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.GUI.Vulkan.buffer.def
    PRIVATE
        common
        external.Vulkan
        lgo.GUI.texture
        lgo.GUI.Vulkan.memory.def
)

target_link_libraries(
    lgo.GUI.Vulkan.buffer.impl
    PRIVATE
        common
        lgo.GUI.Vulkan.def
        external.Vulkan
        lgo.GUI.texture
)
