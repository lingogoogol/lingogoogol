add_library(lgo.GUI.font.FreeType STATIC)

target_sources(
    lgo.GUI.font.FreeType
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.GUI.font.FreeType
    PRIVATE
        common
        external.FreeType
        lgo.io.file
)
