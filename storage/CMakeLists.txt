add_executable(
    storage WIN32
    storage.cpp
)

add_subdirectory(encode)
add_subdirectory(crypto)
add_subdirectory(http)
add_subdirectory(google)

target_link_libraries(
    storage
    PRIVATE
        common
        external.curl
        external.Windows
        external.ShObjIdl
        external.objbase
        external.processenv
        external.shellapi
        lgo.dev
        lgo.container
        lgo.math
        lgo.io
        lgo.GUI
        storage.encode
        storage.http
        storage.google
)

install(
    TARGETS
        storage
    DESTINATION
        ${CMAKE_INSTALL_PREFIX}
)
