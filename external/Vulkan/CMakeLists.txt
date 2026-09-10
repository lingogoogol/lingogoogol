add_library(external.Vulkan STATIC)

find_package(Vulkan REQUIRED)

target_compile_definitions(
    external.Vulkan
    PUBLIC
        VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
)

target_include_directories(
    external.Vulkan
    PRIVATE
        "${Vulkan_INCLUDE_DIR}"
)

target_sources(
    external.Vulkan
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
        FILE_SET vulkan
        TYPE CXX_MODULES
        BASE_DIRS "${Vulkan_INCLUDE_DIR}"
        FILES
            "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
)

target_link_libraries(
    external.Vulkan
    PUBLIC
        Vulkan::Vulkan
)
