add_library(storage.google.batch STATIC)

target_sources(
    storage.google.batch
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    storage.google.batch
    PRIVATE
        common
        external.curl
        lgo.dev.error
        lgo.io.file
        storage.http
        storage.google.account
)
