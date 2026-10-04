add_library(lgo.GUI.object.div_border STATIC)

target_sources(
    lgo.GUI.object.div_border
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

target_link_libraries(
    lgo.GUI.object.div_border
    PRIVATE
        common
        lgo.math.vec
        lgo.GUI.misc
        lgo.GUI.primitive
        lgo.GUI.object.depth_tracker
        lgo.GUI.object.std
        lgo.GUI.object.div
        lgo.GUI.object.rect_border
)
