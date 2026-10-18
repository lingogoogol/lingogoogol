add_executable(
    storage WIN32
    storage.cpp
)

add_subdirectory(encode)
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
