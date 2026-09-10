add_library(lgo.math STATIC)

target_sources(
    lgo.math
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

add_subdirectory(vec)
add_subdirectory(op)
add_subdirectory(ver)

target_link_libraries(
    lgo.math
    PRIVATE
        common
    PUBLIC
        lgo.math.vec
        lgo.math.op
        lgo.math.ver
)
