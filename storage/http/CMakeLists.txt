add_library(storage.http STATIC)

target_sources(
    storage.http
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

add_subdirectory(curl)
add_subdirectory(request)
add_subdirectory(response)

target_link_libraries(
    storage.http
    PRIVATE
        common
    PUBLIC
        storage.http.curl
        storage.http.request
        storage.http.response
)
