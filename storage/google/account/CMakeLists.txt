add_library(storage.google.account STATIC)

target_sources(
    storage.google.account
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    storage.google.account
    PRIVATE
        common
        external.curl
        storage.google.oauth
        storage.encode
        storage.crypto
        storage.http.curl
)
