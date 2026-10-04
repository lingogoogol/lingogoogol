add_library(lgo.GUI.object.menu STATIC)

target_sources(
    lgo.GUI.object.menu
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.GUI.object.menu
    PRIVATE
        common
        lgo.GUI.object.object
        lgo.GUI.object.click_area
        lgo.GUI.object.rect
        lgo.GUI.object.text
)
