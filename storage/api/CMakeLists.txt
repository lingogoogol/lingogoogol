add_library(storage.api OBJECT)

target_sources(
    storage.api
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cpp
)

target_link_libraries(
    storage.api
    PRIVATE
        common
        storage.multipart
)
