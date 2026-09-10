add_library(lgo.GUI.misc STATIC)

target_sources(
    lgo.GUI.misc
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

target_link_libraries(
    lgo.GUI.misc
    PRIVATE
        common
)
