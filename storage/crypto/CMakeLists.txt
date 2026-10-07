add_library(storage.crypto STATIC)

target_sources(
    storage.crypto
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    storage.crypto
    PRIVATE
        common
        external.OpenSSL
)
