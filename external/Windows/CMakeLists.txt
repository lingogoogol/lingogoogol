add_library(external.Windows STATIC)

target_sources(
    external.Windows
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

target_link_libraries(
    external.Windows
    PRIVATE
        common
)
