add_library(lgo.GUI.object.text_scroll STATIC)

target_sources(
    lgo.GUI.object.text_scroll
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.GUI.object.text_scroll
    PRIVATE
        common
        lgo.math.vec
        lgo.GUI.misc
        lgo.GUI.primitive
        lgo.GUI.font
        lgo.GUI.object.depth_tracker
        lgo.GUI.object.object
        lgo.GUI.object.rect
        lgo.GUI.object.text
        lgo.GUI.object.click_area
        lgo.GUI.object.std
)
