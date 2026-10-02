add_library(lgo.GUI.Vulkan.memory.def STATIC)
add_library(lgo.GUI.Vulkan.memory.impl STATIC)

target_sources(
    lgo.GUI.Vulkan.memory.def
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

target_sources(
    lgo.GUI.Vulkan.memory.impl
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.GUI.Vulkan.memory.def
    PRIVATE
        common
        external.Vulkan
)

target_link_libraries(
    lgo.GUI.Vulkan.memory.impl
    PRIVATE
        common
        lgo.GUI.Vulkan.def
        external.Vulkan
)
