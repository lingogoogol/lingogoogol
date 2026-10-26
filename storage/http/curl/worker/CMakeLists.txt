add_library(storage.http.curl.worker STATIC)

target_sources(storage.http.curl.worker
    PUBLIC FILE_SET CXX_MODULES FILES .cppm
    PRIVATE .cpp
)

target_link_libraries(storage.http.curl.worker
    PRIVATE common external.curl storage.http.curl.curl storage.http.curl.easy storage.http.curl.multi
        $<IF:$<BOOL:${MINGW}>,ws2_32,Ws2_32>
)
