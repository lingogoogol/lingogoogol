add_library(storage.google.cloud STATIC)
target_sources(storage.google.cloud PUBLIC FILE_SET CXX_MODULES FILES .cppm PRIVATE .cpp)
target_link_libraries(storage.google.cloud PRIVATE common storage.encode storage.state storage.google.oauth)
