add_library(storage.http.curl.curl STATIC)

target_sources(
    storage.http.curl.curl
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    storage.http.curl.curl
    PRIVATE
        common
        external.curl
        lgo.dev.error
)
