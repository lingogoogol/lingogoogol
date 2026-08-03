add_library(lgo.GUI.engine OBJECT)

target_sources(
    lgo.GUI.engine
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.GUI.engine
    PRIVATE
        common
        external.Vulkan
)
