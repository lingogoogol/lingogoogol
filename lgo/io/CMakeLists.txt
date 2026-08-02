add_library(lgo.io OBJECT)

target_sources(
    lgo.io
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.io
    PRIVATE
        common
)
