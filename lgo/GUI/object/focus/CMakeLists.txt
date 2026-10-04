add_library(lgo.GUI.object.focus STATIC)

target_sources(
    lgo.GUI.object.focus
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.GUI.object.focus
    PRIVATE
        common
)
