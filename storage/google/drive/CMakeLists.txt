add_library(storage.google.drive STATIC)

target_sources(
    storage.google.drive
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

add_subdirectory(blob)
add_subdirectory(blob_queue)

target_link_libraries(
    storage.google.drive
    PRIVATE
        common
    PUBLIC
        storage.google.drive.blob
        storage.google.drive.blob_queue
)
