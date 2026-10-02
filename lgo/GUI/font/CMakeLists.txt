add_library(lgo.GUI.font STATIC)

target_sources(
    lgo.GUI.font
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

add_subdirectory(FreeType)
add_subdirectory(cha)
add_subdirectory(face)
add_subdirectory(preference)

target_link_libraries(
    lgo.GUI.font
    PRIVATE
        common
        lgo.GUI.font.FreeType
        lgo.GUI.font.cha
        lgo.GUI.font.face
        lgo.GUI.font.preference
)
