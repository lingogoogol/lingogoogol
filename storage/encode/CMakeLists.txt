add_library(storage.encode STATIC)

target_sources(
    storage.encode
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    storage.encode
    PRIVATE
        common
        external.Windows
        external.wincrypt
)
