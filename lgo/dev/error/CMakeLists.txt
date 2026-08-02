add_library(lgo.dev.error OBJECT)

target_sources(
    lgo.dev.error
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.dev.error
    PRIVATE
        common
)
