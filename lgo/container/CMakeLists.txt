add_library(lgo.container STATIC)

target_sources(
    lgo.container
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

add_subdirectory(str)

target_link_libraries(
    lgo.container
    PRIVATE
        common
    PUBLIC
        lgo.container.str
)
