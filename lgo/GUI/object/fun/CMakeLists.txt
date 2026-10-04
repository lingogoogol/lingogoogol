add_library(lgo.GUI.object.fun STATIC)

target_sources(
    lgo.GUI.object.fun
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.GUI.object.fun
    PRIVATE
        common
        lgo.GUI.object.state
)
