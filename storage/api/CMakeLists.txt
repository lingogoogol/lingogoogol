add_library(storage.api OBJECT)

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
        external.Windows
        external.wincrypt
        external.OpenSSL
        external.curl
        external.nlohmann
        lgo.io.file
        storage.multipart
)
