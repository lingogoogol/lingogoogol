add_library(external.GLFW OBJECT)

target_include_directories(
    external.GLFW
    PRIVATE
        "${Vulkan_INCLUDE_DIR}"
)

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
