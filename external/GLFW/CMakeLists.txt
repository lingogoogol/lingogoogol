add_library(external.GLFW OBJECT)

find_package(glfw3 CONFIG REQUIRED)

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
    PUBLIC
        common
        external.Vulkan
        glfw
)
