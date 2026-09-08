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
        storage.multipart
        lgo.dev.error
        lgo.io.file
        CURL::libcurl
        Freetype::Freetype
        nlohmann_json::nlohmann_json
        OpenSSL::Crypto
)
