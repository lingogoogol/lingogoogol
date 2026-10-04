add_library(lgo.GUI.object.text_input STATIC)

target_sources(
    lgo.GUI.object.text_input
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

target_link_libraries(
    lgo.GUI.object.text_input
    PRIVATE
        common
        lgo.io.file
        lgo.math.vec
        lgo.GUI.misc
        lgo.GUI.primitive
        lgo.GUI.object.depth_tracker
        lgo.GUI.object.text_scroll
        lgo.GUI.object.click_area
        lgo.GUI.object.std
        lgo.GUI.object.focus
        lgo.GUI.object.text
)
