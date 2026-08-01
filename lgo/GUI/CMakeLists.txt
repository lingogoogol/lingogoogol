add_subdirectory(misc)

add_library(lgo.GUI OBJECT)

target_sources(
    lgo.GUI
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

target_link_libraries(
    lgo.GUI
    PRIVATE
        common
        lgo.GUI.misc
)
