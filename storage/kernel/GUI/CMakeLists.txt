add_library(storage.kernel.GUI STATIC)

target_sources(
    storage.kernel.GUI
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    storage.kernel.GUI
    PRIVATE
        common
        lgo.dev
        lgo.math
        lgo.GUI
        storage.dialog
        storage.http
        storage.google
        storage.kernel.load
)
