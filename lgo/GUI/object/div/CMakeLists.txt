add_library(lgo.GUI.object.div STATIC)

target_sources(
    lgo.GUI.object.div
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.GUI.object.div
    PRIVATE
        common
        lgo.io.file
        lgo.math.vec
        lgo.GUI.misc
        lgo.GUI.primitive
        lgo.GUI.object.object
        lgo.GUI.object.state
        lgo.GUI.object.depth_tracker
)
