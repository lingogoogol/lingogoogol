add_library(storage.state STATIC)
target_sources(storage.state PUBLIC FILE_SET CXX_MODULES FILES .cppm PRIVATE .cpp)
target_link_libraries(storage.state PRIVATE common external.Windows external.OpenSSL ws2_32)
