add_library(lgo.container.str OBJECT)

target_sources(
    lgo.container.str
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.container.str
    PRIVATE
        common
        external.Windows
)
