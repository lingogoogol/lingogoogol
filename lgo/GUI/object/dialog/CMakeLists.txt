add_library(lgo.GUI.object.dialog STATIC)
target_sources(lgo.GUI.object.dialog PUBLIC FILE_SET CXX_MODULES FILES .cppm PRIVATE .cpp)
target_link_libraries(lgo.GUI.object.dialog PRIVATE common lgo.math.vec lgo.GUI.primitive lgo.GUI.object.state lgo.GUI.object.rect lgo.GUI.object.rect_border lgo.GUI.object.text lgo.GUI.object.text_input lgo.GUI.object.button lgo.GUI.object.std)
