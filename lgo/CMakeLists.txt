add_library(lgo OBJECT)

target_sources(
    lgo
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

add_subdirectory(container)
add_subdirectory(math)
add_subdirectory(io)
add_subdirectory(GUI)
add_subdirectory(dev)

target_link_libraries(
    lgo
    PRIVATE
        common
        lgo.container
        lgo.math
        lgo.io
        lgo.GUI
        lgo.dev
)
