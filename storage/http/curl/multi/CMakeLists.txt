add_library(storage.http.curl.multi STATIC)

target_sources(
    storage.http.curl.multi
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    storage.http.curl.multi
    PRIVATE
        common
        external.curl
        lgo.io.file
        lgo.dev.error
        storage.http.curl.curl
        storage.http.curl.easy
)
