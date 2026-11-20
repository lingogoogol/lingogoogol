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
if(ANDROID)
    target_sources(storage.kernel.GUI PRIVATE android.cpp)
endif()

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
        $<$<BOOL:${WIN32}>:windowscodecs>
        $<$<BOOL:${WIN32}>:ole32>
        $<$<BOOL:${WIN32}>:storage.dialog>
        storage.http
        storage.google
        storage.local
        storage.kernel.load
        storage.encode
        storage.kernel.runtime
)
