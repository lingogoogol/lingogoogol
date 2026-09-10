add_library(lgo.io STATIC)

target_sources(
    lgo.io
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

add_subdirectory(stream)
add_subdirectory(file)

target_link_libraries(
    lgo.io
    PRIVATE
        common
    PUBLIC
        lgo.io.stream
        lgo.io.file
)
