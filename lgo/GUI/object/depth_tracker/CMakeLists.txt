add_library(lgo.GUI.object.depth_tracker STATIC)

target_sources(
    lgo.GUI.object.depth_tracker
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.GUI.object.depth_tracker
    PRIVATE
        common
        lgo.math.vec
        lgo.math.op
)
