add_library(lgo.GUI.texture.pixmap.def STATIC)
add_library(lgo.GUI.texture.pixmap.impl STATIC)

target_sources(
    lgo.GUI.texture.pixmap.def
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

target_sources(
    lgo.GUI.texture.pixmap.impl
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.GUI.texture.pixmap.def
    PRIVATE
        common
        lgo.math.vec
)

target_link_libraries(
    lgo.GUI.texture.pixmap.impl
    PRIVATE
        common
        lgo.GUI.texture.def
        lgo.math.vec
)
