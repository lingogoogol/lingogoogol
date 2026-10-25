add_library(lgo.GUI.Vulkan.debug.def STATIC)
target_sources(lgo.GUI.Vulkan.debug.def PUBLIC FILE_SET CXX_MODULES FILES .cppm)
target_link_libraries(lgo.GUI.Vulkan.debug.def PRIVATE common external.Vulkan external.Windows)
