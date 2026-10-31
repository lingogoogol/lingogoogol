add_library(storage.kernel.GUI STATIC)

target_sources(
    storage.kernel.GUI
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    storage.kernel.GUI
    PRIVATE
        common
        external.Vulkan
        external.curl
        external.GLFW
        lgo.io.file
        lgo.dev
        lgo.math
        lgo.GUI
        windowscodecs
        ole32
        storage.dialog
        storage.http
        storage.google
        storage.kernel.load
        storage.kernel.runtime
)
