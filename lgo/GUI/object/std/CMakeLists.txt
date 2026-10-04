add_library(lgo.GUI.object.std STATIC)

target_sources(
    lgo.GUI.object.std
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

target_link_libraries(
    lgo.GUI.object.std
    PRIVATE
        common
        lgo.math.vec
        lgo.GUI.misc
)
