add_library(lgo.GUI.primitive INTERFACE)
add_library(lgo.GUI.primitive.def STATIC)

target_sources(
    lgo.GUI.primitive.def
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

add_subdirectory(engine)
add_subdirectory(rect)

target_link_libraries(
    lgo.GUI.primitive.def
    PRIVATE
        common
        lgo.GUI.primitive.engine.def
        lgo.GUI.primitive.rect.def
)

target_link_libraries(
    lgo.GUI.primitive
    INTERFACE
        lgo.GUI.primitive.def
        lgo.GUI.primitive.engine.impl
        lgo.GUI.primitive.rect.impl
)
