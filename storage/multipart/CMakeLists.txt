add_library(storage.multipart OBJECT)

target_sources(
    storage.multipart
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cpp
)

target_link_libraries(
    storage.multipart
    PRIVATE
        common
)
