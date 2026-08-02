add_library(lgo.container OBJECT)

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
        lgo.container.str
)
