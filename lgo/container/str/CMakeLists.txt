add_library(lgo.container.str STATIC)

target_sources(
    lgo.container.str
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.container.str
    PRIVATE
        common
    PUBLIC
        $<$<BOOL:${WIN32}>:external.Windows>
)
