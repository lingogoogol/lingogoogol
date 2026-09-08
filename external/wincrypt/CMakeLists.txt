add_library(external.wincrypt OBJECT)

target_sources(
    external.wincrypt
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

target_link_libraries(
    external.wincrypt
    PRIVATE
        common
)
