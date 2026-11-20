if(ANDROID)
    # GameActivity loads the app as libstorage.so; its entry point is in android/native.
    add_library(
        storage SHARED
        storage.cpp
    )
    target_link_libraries(storage PRIVATE android.app)
    # GameActivity's JNI entry points live in a static library; keep them exported.
    target_link_options(storage PRIVATE "-u" "Java_com_google_androidgamesdk_GameActivity_initializeNativeCode")
else()
    add_executable(
        storage WIN32
        storage.cpp
    )
endif()

add_subdirectory(state)
add_subdirectory(encode)
add_subdirectory(local)
# `storage.cpp` uses `wWinMain` as its entry point; MinGW requires `-municode` to select the wide-char CRT startup.
target_link_options(
    storage
    PRIVATE
        $<$<BOOL:${MINGW}>:-municode>
)
add_subdirectory(crypto)
if(WIN32)
    add_subdirectory(dialog)
endif()
add_subdirectory(http)
add_subdirectory(google)
add_subdirectory(kernel)

target_link_libraries(
    storage
    PRIVATE
        common
        $<$<BOOL:${WIN32}>:external.Windows>
        external.Vulkan
        lgo.container.str
        $<$<BOOL:${WIN32}>:external.processenv>
        $<$<BOOL:${WIN32}>:external.shellapi>
        lgo.math
        lgo.io
        lgo.GUI
        storage.http
        storage.encode
        storage.google
        storage.kernel
        $<$<BOOL:${ANDROID}>:storage.local>
)

if(NOT ANDROID)
    install(
        TARGETS
            storage
        DESTINATION
            bin
    )
endif()

if(MINGW AND CMAKE_FIND_ROOT_PATH)
    install(
        FILES
            "${CMAKE_FIND_ROOT_PATH}/bin/libc++.dll"
            "${CMAKE_FIND_ROOT_PATH}/bin/libunwind.dll"
            "${CMAKE_FIND_ROOT_PATH}/bin/glfw3.dll"
            "${CMAKE_FIND_ROOT_PATH}/bin/libcurl.dll"
            "${CMAKE_FIND_ROOT_PATH}/bin/libcrypto-3-x64.dll"
            "${CMAKE_FIND_ROOT_PATH}/bin/libssl-3-x64.dll"
        DESTINATION
            bin
    )
endif()
