add_library(lgo.dev OBJECT)

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
        lgo.dev.error
)
