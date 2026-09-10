add_library(lgo.dev STATIC)

target_sources(
    lgo.dev
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

add_subdirectory(error)

target_link_libraries(
    lgo.dev
    PRIVATE
        common
    PUBLIC
        lgo.dev.error
)
