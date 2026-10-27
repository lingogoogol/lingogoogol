add_library(storage.kernel.runtime STATIC)
target_sources(storage.kernel.runtime PUBLIC FILE_SET CXX_MODULES FILES .cppm PRIVATE .cpp)
target_link_libraries(storage.kernel.runtime PRIVATE common storage.kernel.load storage.google storage.http storage.state)
