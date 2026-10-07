add_library(storage.google.drive.blob STATIC)

target_sources(
    storage.google.drive.blob
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    storage.google.drive.blob
    PRIVATE
        common
        storage.http
        storage.google.batch
)
