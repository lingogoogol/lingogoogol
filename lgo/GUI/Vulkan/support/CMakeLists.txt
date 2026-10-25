add_library(lgo.GUI.Vulkan.support.def STATIC)
target_sources(lgo.GUI.Vulkan.support.def PUBLIC FILE_SET CXX_MODULES FILES .cppm)
target_link_libraries(lgo.GUI.Vulkan.support.def PRIVATE common external.Vulkan)
