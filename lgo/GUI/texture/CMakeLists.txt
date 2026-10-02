add_library(lgo.GUI.texture INTERFACE)
add_library(lgo.GUI.texture.def STATIC)

target_sources(
    lgo.GUI.texture.def
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

add_subdirectory(pixmap)

target_link_libraries(
    lgo.GUI.texture.def
    PRIVATE
        common
        lgo.GUI.texture.pixmap.def
)

target_link_libraries(
    lgo.GUI.texture
    INTERFACE
        lgo.GUI.texture.def
        lgo.GUI.texture.pixmap.impl
)
