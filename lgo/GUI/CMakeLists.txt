add_library(lgo.GUI STATIC)

target_sources(
    lgo.GUI
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

add_subdirectory(misc)
add_subdirectory(Vulkan)
add_subdirectory(primitive)
add_subdirectory(texture)
add_subdirectory(font)
add_subdirectory(object)

target_link_libraries(
    lgo.GUI
    PRIVATE
        common
    PUBLIC
        lgo.GUI.misc
        lgo.GUI.Vulkan
        lgo.GUI.primitive
        lgo.GUI.texture
        lgo.GUI.font
        lgo.GUI.object
)
