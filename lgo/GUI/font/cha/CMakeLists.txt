add_library(lgo.GUI.font.cha STATIC)

target_sources(
    lgo.GUI.font.cha
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.GUI.font.cha
    PRIVATE
        common
        external.FreeType
        lgo.math.vec
        lgo.GUI.font.face
        lgo.GUI.font.preference
        lgo.GUI.texture
)
