add_library(lgo.io.file OBJECT)

target_sources(
    lgo.io.file
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.io.file
    PRIVATE
        common
        external.Windows
        lgo.dev.error
)
