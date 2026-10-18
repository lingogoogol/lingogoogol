add_library(storage.dialog STATIC)

target_sources(
    storage.dialog
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    storage.dialog
    PRIVATE
        common
        external.Windows
        external.ShObjIdl
        external.objbase
        lgo.container.str
)
