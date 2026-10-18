add_library(lgo.GUI.object.text STATIC)

target_sources(
    lgo.GUI.object.text
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.GUI.object.text
    PRIVATE
        common
        external.Vulkan
        lgo.container.str
        lgo.io.file
        lgo.math.vec
        lgo.GUI.misc
        lgo.GUI.Vulkan
        lgo.GUI.primitive
        lgo.GUI.texture
        lgo.GUI.font
        lgo.GUI.object.object
        lgo.GUI.object.depth_tracker
        lgo.GUI.object.std
        lgo.GUI.object.virt
        lgo.GUI.object.div
        lgo.GUI.object.rect
)
