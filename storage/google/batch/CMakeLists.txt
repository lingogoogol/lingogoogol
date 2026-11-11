add_library(storage.google.batch STATIC)

target_sources(
    storage.google.batch
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    storage.google.batch
    PRIVATE
        common
        external.curl
        lgo.dev.error
        lgo.io.file
        storage.http
        storage.google.easy
        storage.google.account
)

find_package(ZLIB)
if(ZLIB_FOUND)
    target_link_libraries(storage.google.batch PRIVATE ZLIB::ZLIB)
    target_compile_definitions(storage.google.batch PRIVATE LGO_HAVE_ZLIB=1)
endif()
