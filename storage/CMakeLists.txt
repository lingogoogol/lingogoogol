add_executable(storage WIN32 storage.cpp)

target_link_libraries(
    storage
    PRIVATE
        common
)
