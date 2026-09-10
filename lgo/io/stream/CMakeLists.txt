add_library(lgo.io.stream STATIC)

target_sources(
    lgo.io.stream
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.io.stream
    PRIVATE
        common
)
