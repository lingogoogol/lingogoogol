add_library(external.OpenSSL STATIC)

target_sources(
    external.OpenSSL
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

target_link_libraries(
    external.OpenSSL
    PRIVATE
        common
        $<IF:$<BOOL:${MINGW}>,ssl,libssl>
        $<IF:$<BOOL:${MINGW}>,crypto,libcrypto>
)
