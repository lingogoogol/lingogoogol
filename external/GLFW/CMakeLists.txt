add_library(external.GLFW OBJECT)

target_sources(
    external.GLFW
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

target_link_libraries(
    external.GLFW
    PRIVATE
        common
)
