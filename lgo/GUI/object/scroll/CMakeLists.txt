add_library(lgo.GUI.object.scroll STATIC)
target_sources(lgo.GUI.object.scroll PUBLIC FILE_SET CXX_MODULES FILES .cppm)
target_link_libraries(lgo.GUI.object.scroll PRIVATE common external.GLFW
    lgo.math.vec lgo.math.op lgo.GUI.misc lgo.GUI.primitive lgo.GUI.object.object
    lgo.GUI.object.depth_tracker lgo.GUI.object.rect)
