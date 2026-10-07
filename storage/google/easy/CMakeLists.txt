add_library(storage.google.easy STATIC)

target_sources(
    storage.google.easy
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    storage.google.easy
    PRIVATE
        common
        storage.google.account
)
