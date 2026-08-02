add_library(lgo.math.ver OBJECT)

target_sources(
    lgo.math.ver
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.math.ver
    PRIVATE
        common
)
