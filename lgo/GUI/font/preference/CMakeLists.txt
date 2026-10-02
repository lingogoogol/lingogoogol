add_library(lgo.GUI.font.preference STATIC)

target_sources(
    lgo.GUI.font.preference
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.GUI.font.preference
    PRIVATE
        common
        external.FreeType
        lgo.GUI.font.face
)
