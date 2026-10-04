add_library(lgo.GUI.object.rect STATIC)

target_sources(
    lgo.GUI.object.rect
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.GUI.object.rect
    PRIVATE
        common
        lgo.math.vec
        lgo.math.op
        lgo.io.file
        lgo.GUI.misc
        lgo.GUI.Vulkan
        lgo.GUI.primitive
        lgo.GUI.object.object
        lgo.GUI.object.std
        lgo.GUI.object.depth_tracker
)
