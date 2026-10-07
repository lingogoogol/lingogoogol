add_library(storage.http.response STATIC)

target_sources(
    storage.http.response
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    storage.http.response
    PRIVATE
        common
)
