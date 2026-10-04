add_library(lgo.GUI.object.object STATIC)

target_sources(
    lgo.GUI.object.object
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.GUI.object.object
    PRIVATE
        common
        lgo.io.file
        lgo.math.vec
)
