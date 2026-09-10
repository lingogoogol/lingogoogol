add_library(external.objbase STATIC)

target_sources(
    external.objbase
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

target_link_libraries(
    external.objbase
    PRIVATE
        common
        Ole32.lib
)
