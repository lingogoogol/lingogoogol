add_library(storage.encode STATIC)

target_sources(
    storage.encode
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
        content.cpp
)

# Optional, as with batch gzip's ZLIB dependency.
find_package(zstd QUIET CONFIG)
if(zstd_FOUND)
    # Keep the compiler's own CRT headers when the dependency uses another
    # MinGW distribution. Stage just zstd's public headers.
    get_target_property(zstd_include_dirs zstd::libzstd_shared INTERFACE_INCLUDE_DIRECTORIES)
    find_file(zstd_header zstd.h HINTS ${zstd_include_dirs} REQUIRED)
    configure_file("${zstd_header}" "zstd-include/zstd.h" COPYONLY)
    get_filename_component(zstd_header_dir "${zstd_header}" DIRECTORY)
    configure_file("${zstd_header_dir}/zstd_errors.h" "zstd-include/zstd_errors.h" COPYONLY)
    target_include_directories(storage.encode PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/zstd-include")
    target_link_libraries(storage.encode PRIVATE "$<LINK_ONLY:zstd::libzstd_shared>")
    target_compile_definitions(storage.encode PRIVATE LGO_HAVE_ZSTD=1)
endif()

if(LGO_BUILD_CONTENT_TESTS)
    add_executable(content-codec-test "${PROJECT_SOURCE_DIR}/tests/content_codec.cpp")
    target_link_libraries(content-codec-test PRIVATE common storage.encode storage.state)
    add_test(NAME content-codec COMMAND content-codec-test)
endif()

target_link_libraries(
    storage.encode
    PRIVATE
        common
        external.Windows
        external.wincrypt
)
