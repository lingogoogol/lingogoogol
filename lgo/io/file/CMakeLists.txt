add_library(lgo.io.file STATIC)

target_sources(
    lgo.io.file
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
    PRIVATE
        .cpp
)

target_link_libraries(
    lgo.io.file
    PRIVATE
        common
    PUBLIC
        $<$<BOOL:${WIN32}>:external.Windows>
        $<$<BOOL:${ANDROID}>:log>
        lgo.dev.error
        lgo.container.str
)
