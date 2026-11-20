add_library(external.FreeType STATIC)

target_sources(
    external.FreeType
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

target_link_libraries(
    external.FreeType
    PRIVATE
        common
        # The Android dependency script builds only the release library.
        $<IF:$<AND:$<CONFIG:Debug>,$<NOT:$<BOOL:${ANDROID}>>>,freetyped,freetype>
)
