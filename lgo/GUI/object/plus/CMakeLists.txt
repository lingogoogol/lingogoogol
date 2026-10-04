add_library(lgo.GUI.object.plus STATIC)

target_sources(
    lgo.GUI.object.plus
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.GUI.object.plus
    PRIVATE
        common
        lgo.math.vec
        lgo.GUI.misc
        lgo.GUI.primitive
        lgo.GUI.object.object
        lgo.GUI.object.click_area
        lgo.GUI.object.rect
        lgo.GUI.object.rect_border
        lgo.GUI.object.text
        lgo.GUI.object.depth_tracker
        lgo.GUI.object.std
)
