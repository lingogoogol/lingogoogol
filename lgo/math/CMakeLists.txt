add_subdirectory(ver)

add_library(lgo.math OBJECT)

target_sources(
    lgo.math
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

target_link_libraries(
    storage
    PRIVATE
        common
        lgo.math.ver
)
