add_library(external.ShObjIdl STATIC)

target_sources(
    external.ShObjIdl
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

target_link_libraries(
    external.ShObjIdl
    PRIVATE
        common
)
