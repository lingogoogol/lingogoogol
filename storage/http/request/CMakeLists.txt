add_library(storage.http.request STATIC)

target_sources(
    storage.http.request
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    storage.http.request
    PRIVATE
        common
)
