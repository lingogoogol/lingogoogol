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

find_package(ZLIB QUIET)
if(NOT ZLIB_FOUND AND NOT CMAKE_DISABLE_FIND_PACKAGE_ZLIB)
    # The workspace prefix contains Windows cross-built libraries. Explicitly
    # bypass sysroot re-rooting only for this prefix, never for Linux libraries.
    set(storage_zlib_prefix "${PROJECT_SOURCE_DIR}/../lib/zlib")
    find_path(ZLIB_INCLUDE_DIR zlib.h PATHS "${storage_zlib_prefix}/include"
        NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    find_library(ZLIB_LIBRARY_RELEASE NAMES zlib z PATHS "${storage_zlib_prefix}/lib"
        NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    find_package(ZLIB)
endif()
if(ZLIB_FOUND)
    target_link_libraries(storage.google.batch PRIVATE ZLIB::ZLIB)
    target_compile_definitions(storage.google.batch PRIVATE LGO_HAVE_ZLIB=1)
endif()

if(MINGW AND ZLIB_FOUND)
    # FindZLIB exposes the import library, but does not identify its runtime DLL.
    find_file(storage_zlib_runtime NAMES libzlib.dll zlib1.dll zlib.dll libz.dll
        HINTS "${ZLIB_INCLUDE_DIR}/../bin" "${CMAKE_FIND_ROOT_PATH}/bin"
        NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    if(storage_zlib_runtime)
        install(FILES "${storage_zlib_runtime}" DESTINATION bin)
    endif()
endif()
