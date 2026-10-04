add_library(lgo.GUI.object.button STATIC)

target_sources(
    lgo.GUI.object.button
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.GUI.object.button
    PRIVATE
        common
        lgo.io.file
        lgo.math.vec
        lgo.GUI.misc
        lgo.GUI.primitive
        lgo.GUI.font
        lgo.GUI.object.object
        lgo.GUI.object.depth_tracker
        lgo.GUI.object.rect_border
        lgo.GUI.object.text
        lgo.GUI.object.click_area
        lgo.GUI.object.std
)
