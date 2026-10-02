add_library(lgo.GUI.font.face STATIC)

target_sources(
    lgo.GUI.font.face
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.GUI.font.face
    PRIVATE
        common
        external.FreeType
        lgo.GUI.font.FreeType
)
