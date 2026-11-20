add_library(storage.state STATIC)
target_sources(storage.state PUBLIC FILE_SET CXX_MODULES FILES .cppm PRIVATE .cpp io.cpp)
target_link_libraries(storage.state PRIVATE common $<$<BOOL:${WIN32}>:external.Windows> external.OpenSSL $<$<BOOL:${WIN32}>:ws2_32>)
