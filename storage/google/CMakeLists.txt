add_library(storage.google STATIC)

target_sources(
    storage.google
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

add_subdirectory(oauth)
add_subdirectory(account)
add_subdirectory(easy)
add_subdirectory(batch)
add_subdirectory(drive)

target_link_libraries(
    storage.google
    PRIVATE
        common
    PUBLIC
        storage.google.oauth
        storage.google.account
        storage.google.easy
        storage.google.batch
        storage.google.drive
)
