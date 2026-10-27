add_library(storage.google.drive.blob_queue STATIC)

target_sources(
    storage.google.drive.blob_queue
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    storage.google.drive.blob_queue
    PRIVATE
        common
        lgo.io.file
        storage.http.curl.worker
        storage.http.curl
        storage.google.account
        storage.google.batch
        storage.google.drive.blob
)
