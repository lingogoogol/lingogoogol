add_library(lgo.GUI.object.state STATIC)

target_sources(
    lgo.GUI.object.state
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.GUI.object.state
    PRIVATE
        common
        lgo.io.file
        lgo.GUI.primitive
        lgo.GUI.object.object
        lgo.GUI.object.focus
        lgo.GUI.object.depth_tracker
)
