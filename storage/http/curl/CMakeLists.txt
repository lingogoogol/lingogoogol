add_library(storage.http.curl STATIC)

target_sources(
    storage.http.curl
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

add_subdirectory(curl)
add_subdirectory(easy)
add_subdirectory(multi)

target_link_libraries(
    storage.http.curl
    PRIVATE
        common
    PUBLIC
        storage.http.curl.curl
        storage.http.curl.easy
        storage.http.curl.multi
)
