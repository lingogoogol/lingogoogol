add_executable(
    storage WIN32
    storage.cpp
)

add_subdirectory(multipart)
add_subdirectory(api)

target_link_libraries(
    storage
    PRIVATE
        common
    PUBLIC
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
        storage.multipart
        storage.api
)

install(
    TARGETS
        storage
    DESTINATION
        ${CMAKE_INSTALL_PREFIX}
)
