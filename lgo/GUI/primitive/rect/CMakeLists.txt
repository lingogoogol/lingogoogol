add_library(lgo.GUI.primitive.rect.def STATIC)
add_library(lgo.GUI.primitive.rect.impl STATIC)

target_sources(
    lgo.GUI.primitive.rect.def
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

target_sources(
    lgo.GUI.primitive.rect.impl
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.GUI.primitive.rect.def
    PRIVATE
        common
        external.Vulkan
        lgo.math.vec
        lgo.math.op
        lgo.GUI.misc
        lgo.GUI.Vulkan
        lgo.GUI.primitive.engine.decl
)

target_link_libraries(
    lgo.GUI.primitive.rect.impl
    PRIVATE
        common
        lgo.GUI.primitive.def
        external.Vulkan
        lgo.io.file
        lgo.math.vec
        lgo.math.op
        lgo.GUI.misc
        lgo.GUI.Vulkan
)
