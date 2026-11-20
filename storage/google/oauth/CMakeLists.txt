add_library(storage.google.oauth STATIC)
if(ANDROID)
    # Windows CNG keys and the loopback redirect are Windows-only; see android.cpp.
    target_sources(storage.google.oauth PUBLIC FILE_SET CXX_MODULES FILES .cppm PRIVATE android.cpp)
    target_link_libraries(storage.google.oauth PRIVATE common storage.state external.curl)
else()
    target_sources(storage.google.oauth PUBLIC FILE_SET CXX_MODULES FILES .cppm PRIVATE .cpp)
    target_link_libraries(storage.google.oauth PRIVATE common storage.state storage.encode storage.http.curl.easy
        ncrypt bcrypt shell32 ws2_32 external.curl external.OpenSSL)
endif()
