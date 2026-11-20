# Included by the top-level CMakeLists.txt after `project()` for Android builds.

# Bionic declares the <ctype.h> functions and the _FORTIFY_SOURCE wrappers of
# <stdio.h>/<string.h> (memcpy, snprintf, fread, ...) as `static inline`. The
# `std` module re-exports these names, and Clang rejects exporting entities with
# internal linkage. Give the ctype functions external linkage and use the plain
# libc declarations instead of the fortified wrappers. This must reach the
# synthesized `std` module target too, so it is a global flag, not a target option.
string(APPEND CMAKE_CXX_FLAGS " -U_FORTIFY_SOURCE -D__BIONIC_CTYPE_INLINE=inline")

# Android has no sysroot shared with third-party libraries. The dependencies are
# built per ABI by android/scripts/build-dependencies.sh into
# <prefix>/<ABI>/{include,lib}; by default beside the Windows libraries in ../lib.
set(LGO_ANDROID_DEPENDENCIES "${CMAKE_CURRENT_LIST_DIR}/../../../lib/android" CACHE PATH
    "Root of the per-ABI Android dependency prefixes.")
get_filename_component(LGO_ANDROID_DEPENDENCY_PREFIX "${LGO_ANDROID_DEPENDENCIES}/${ANDROID_ABI}" ABSOLUTE)
if(NOT EXISTS "${LGO_ANDROID_DEPENDENCY_PREFIX}/include/curl/curl.h")
    message(FATAL_ERROR "Android dependencies for ${ANDROID_ABI} are missing in ${LGO_ANDROID_DEPENDENCY_PREFIX}. "
                        "Run android/scripts/build-dependencies.sh first.")
endif()
list(APPEND CMAKE_FIND_ROOT_PATH "${LGO_ANDROID_DEPENDENCY_PREFIX}")

# The NDK sysroot carries older Vulkan C headers without Vulkan-Hpp's `vulkan`
# module; use the Vulkan-Headers installed with the dependencies and the
# platform's libvulkan.so.
set(Vulkan_INCLUDE_DIR "${LGO_ANDROID_DEPENDENCY_PREFIX}/include" CACHE PATH "Vulkan-Headers for Android." FORCE)
find_library(Vulkan_LIBRARY vulkan REQUIRED)
