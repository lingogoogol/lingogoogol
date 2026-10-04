add_library(lgo.GUI.object.rect_border STATIC)

target_sources(
    lgo.GUI.object.rect_border
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.GUI.object.rect_border
    PRIVATE
        common
        lgo.io.file
        lgo.math.vec
        lgo.GUI.misc
        lgo.GUI.primitive
        lgo.GUI.object.object
        lgo.GUI.object.std
        lgo.GUI.object.depth_tracker
        lgo.GUI.object.rect
)
