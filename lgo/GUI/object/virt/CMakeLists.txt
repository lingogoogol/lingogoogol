add_library(lgo.GUI.object.virt STATIC)

target_sources(
    lgo.GUI.object.virt
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

target_link_libraries(
    lgo.GUI.object.virt
    PRIVATE
        common
        lgo.math.vec
        lgo.GUI.primitive
        lgo.GUI.object.object
        lgo.GUI.object.depth_tracker
)
