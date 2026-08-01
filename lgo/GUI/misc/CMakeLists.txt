add_library(lgo.GUI.misc OBJECT)

target_sources(
    lgo.GUI.misc
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)
