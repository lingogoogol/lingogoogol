add_library(storage.kernel.load STATIC)

target_sources(
    storage.kernel.load
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
        directory.cpp
)

target_link_libraries(
    storage.kernel.load
    PRIVATE
        common
        external.curl
        lgo.dev
        lgo.io
        lgo.GUI
        storage.encode
        storage.http
        storage.google
)
