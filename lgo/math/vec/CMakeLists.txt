add_library(lgo.math.vec STATIC)

target_sources(
    lgo.math.vec
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.math.vec
    PRIVATE
        common
)
