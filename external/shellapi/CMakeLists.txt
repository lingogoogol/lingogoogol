add_library(external.shellapi STATIC)

target_sources(
    external.shellapi
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

target_link_libraries(
    external.shellapi
    PRIVATE
        common
        Shell32.lib
)
