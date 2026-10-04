add_library(lgo.GUI.object.click_area STATIC)

target_sources(
    lgo.GUI.object.click_area
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.GUI.object.click_area
    PRIVATE
        common
        external.GLFW
        lgo.io.file
        lgo.math.vec
        lgo.math.op
        lgo.GUI.misc
        lgo.GUI.primitive
        lgo.GUI.object.object
        lgo.GUI.object.depth_tracker
)
