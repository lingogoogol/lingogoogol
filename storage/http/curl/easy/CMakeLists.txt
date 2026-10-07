add_library(storage.http.curl.easy STATIC)

target_sources(
    storage.http.curl.easy
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    storage.http.curl.easy
    PRIVATE
        common
        external.curl
        lgo.dev
        lgo.io.file
        storage.http.curl.curl
)
