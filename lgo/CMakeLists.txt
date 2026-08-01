add_subdirectory(io)
add_subdirectory(math)

add_library(lgo OBJECT)

target_sources(
    lgo
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

target_link_libraries(
    storage
    PRIVATE
        common
        lgo.io
        lgo.math
)
