add_executable(
    storage WIN32
    storage.cpp
)

add_subdirectory(encode)
# `storage.cpp` uses `wWinMain` as its entry point; MinGW requires `-municode` to select the wide-char CRT startup.
target_link_options(
    storage
    PRIVATE
        $<$<BOOL:${MINGW}>:-municode>
)
add_subdirectory(crypto)
add_subdirectory(dialog)
add_subdirectory(http)
add_subdirectory(google)
add_subdirectory(kernel)

target_link_libraries(
    storage
    PRIVATE
        common
        external.Windows
        external.Vulkan
        lgo.container.str
        external.processenv
        external.shellapi
        lgo.math
        lgo.io
        lgo.GUI
        storage.http
        storage.google
        storage.kernel
)

install(
    TARGETS
        storage
    DESTINATION
        ${CMAKE_INSTALL_PREFIX}
)

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
            ${CMAKE_INSTALL_PREFIX}
    )
endif()
