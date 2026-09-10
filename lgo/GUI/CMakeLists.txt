add_library(lgo.GUI STATIC)

target_sources(
    lgo.GUI
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

add_subdirectory(misc)
add_subdirectory(engine)

target_link_libraries(
    lgo.GUI
    PRIVATE
        common
    PUBLIC
        lgo.GUI.misc
        lgo.GUI.engine
)
