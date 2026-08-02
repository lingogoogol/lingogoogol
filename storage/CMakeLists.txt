add_executable(
    storage WIN32
    storage.cpp
)

add_subdirectory(multipart)
add_subdirectory(api)

target_link_libraries(
    storage
    PRIVATE
        common
        storage.multipart
        storage.api
)
