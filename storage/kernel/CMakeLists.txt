add_library(storage.kernel STATIC)

target_sources(
    storage.kernel
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

add_subdirectory(load)
add_subdirectory(GUI)

target_link_libraries(
    storage.kernel
    PRIVATE
        common
    PUBLIC
        storage.kernel.load
        storage.kernel.GUI
)
