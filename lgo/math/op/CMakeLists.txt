add_library(lgo.math.op STATIC)

target_sources(
    lgo.math.op
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.math.op
    PRIVATE
        common
    PUBLIC
        lgo.math.vec
)
