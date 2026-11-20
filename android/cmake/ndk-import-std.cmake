# Included by the top-level CMakeLists.txt before `project()` for Android builds.
#
# The NDK ships libc++'s `std.cppm`, but only installs `libc++.modules.json` for
# its host toolchains, not for Android targets. Without a manifest, CMake's
# `import std;` detection finds the build host's GCC `bits/std.cc` instead and
# fails. Write a manifest that points at the NDK's own libc++ module sources.
# Paths must be absolute because they depend on where the NDK is installed.

if(CMAKE_VERSION VERSION_LESS 4.2)
    message(FATAL_ERROR "Android builds need CMake 4.2 or newer for CMAKE_CXX_STDLIB_MODULES_JSON; "
                        "the Android SDK's CMake packages are older. Put CMake 4.2+ on PATH.")
endif()

if(DEFINED ANDROID_NDK AND NOT ANDROID_NDK STREQUAL "")
    set(lgo_ndk_root "${ANDROID_NDK}")
else()
    # <ndk>/build/cmake/android.toolchain.cmake
    get_filename_component(lgo_ndk_root "${CMAKE_TOOLCHAIN_FILE}/../../.." ABSOLUTE)
endif()

file(GLOB lgo_ndk_std_module "${lgo_ndk_root}/toolchains/llvm/prebuilt/*/share/libc++/v1/std.cppm")
list(LENGTH lgo_ndk_std_module lgo_ndk_std_module_count)
if(NOT lgo_ndk_std_module_count EQUAL 1)
    message(FATAL_ERROR "Expected one libc++ std.cppm in the NDK at ${lgo_ndk_root}; found: ${lgo_ndk_std_module}")
endif()
get_filename_component(lgo_ndk_libcxx_modules "${lgo_ndk_std_module}" DIRECTORY)

set(lgo_ndk_modules_json "${CMAKE_BINARY_DIR}/ndk-libc++.modules.json")
file(WRITE "${lgo_ndk_modules_json}" "{
  \"version\": 1,
  \"revision\": 1,
  \"modules\": [
    {
      \"logical-name\": \"std\",
      \"source-path\": \"${lgo_ndk_libcxx_modules}/std.cppm\",
      \"is-std-library\": true,
      \"local-arguments\": { \"system-include-directories\": [ \"${lgo_ndk_libcxx_modules}\" ] }
    },
    {
      \"logical-name\": \"std.compat\",
      \"source-path\": \"${lgo_ndk_libcxx_modules}/std.compat.cppm\",
      \"is-std-library\": true,
      \"local-arguments\": { \"system-include-directories\": [ \"${lgo_ndk_libcxx_modules}\" ] }
    }
  ]
}
")
set(CMAKE_CXX_STDLIB_MODULES_JSON "${lgo_ndk_modules_json}" CACHE FILEPATH
    "libc++ module manifest for the Android NDK (generated)." FORCE)
