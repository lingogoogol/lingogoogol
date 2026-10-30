add_library(storage.google.oauth STATIC)
target_sources(storage.google.oauth PUBLIC FILE_SET CXX_MODULES FILES .cppm PRIVATE .cpp)
target_link_libraries(storage.google.oauth PRIVATE common storage.state storage.encode
    ncrypt bcrypt shell32 ws2_32 external.curl external.OpenSSL)
