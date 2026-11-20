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

# Versioned content always uses zstd; prefer a static library when available.
find_package(zstd QUIET CONFIG)
if(NOT zstd_FOUND)
    # This workspace directory contains Windows cross-built dependencies; do
    # not let CMake re-root it into the compiler sysroot or find Linux libraries.
    find_package(zstd REQUIRED CONFIG PATHS "${PROJECT_SOURCE_DIR}/../lib/zstd/lib/cmake/zstd"
        NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
endif()
if(zstd_FOUND)
    # Keep the compiler's own CRT headers when the dependency uses another
    # MinGW distribution. Stage just zstd's public headers.
    if(TARGET zstd::libzstd_static)
        set(storage_zstd_target zstd::libzstd_static)
    else()
        set(storage_zstd_target zstd::libzstd_shared)
    endif()
    get_target_property(zstd_include_dirs ${storage_zstd_target} INTERFACE_INCLUDE_DIRECTORIES)
    find_file(zstd_header zstd.h HINTS ${zstd_include_dirs} NO_CMAKE_FIND_ROOT_PATH REQUIRED)
    configure_file("${zstd_header}" "zstd-include/zstd.h" COPYONLY)
    get_filename_component(zstd_header_dir "${zstd_header}" DIRECTORY)
    configure_file("${zstd_header_dir}/zstd_errors.h" "zstd-include/zstd_errors.h" COPYONLY)
    target_include_directories(storage.encode PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/zstd-include")
    target_link_libraries(storage.encode PRIVATE "$<LINK_ONLY:${storage_zstd_target}>")
    target_compile_definitions(storage.encode PRIVATE LGO_HAVE_ZSTD=1)
endif()

if(LGO_BUILD_CONTENT_TESTS)
    add_executable(content-codec-test "${PROJECT_SOURCE_DIR}/tests/content_codec.cpp")
    target_link_libraries(content-codec-test PRIVATE common storage.encode storage.state)
    add_test(NAME content-codec COMMAND content-codec-test)
    add_executable(folder-metadata-test "${PROJECT_SOURCE_DIR}/tests/folder_metadata.cpp")
    target_link_libraries(folder-metadata-test PRIVATE common)
    add_test(NAME folder-metadata COMMAND folder-metadata-test)
    add_executable(async-state-test "${PROJECT_SOURCE_DIR}/tests/async_state.cpp")
    target_link_libraries(async-state-test PRIVATE common storage.state)
    add_test(NAME async-state COMMAND async-state-test)
endif()

target_link_libraries(
    storage.encode
    PRIVATE
        common
        $<$<BOOL:${WIN32}>:external.Windows>
        $<$<BOOL:${WIN32}>:external.wincrypt>
)
