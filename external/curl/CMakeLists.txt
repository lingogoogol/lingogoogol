add_library(external.curl STATIC)

target_sources(
    external.curl
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

target_link_libraries(
    external.curl
    PRIVATE
        common
        libcurl.lib
)
