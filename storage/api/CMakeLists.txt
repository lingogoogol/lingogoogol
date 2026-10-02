add_library(storage.api STATIC)

target_sources(
    storage.api
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

target_link_libraries(
    storage.api
    PRIVATE
        common
    PUBLIC
        external.Windows
        external.wincrypt
        external.OpenSSL
        external.curl
        lgo.io.file
        storage.multipart
)
