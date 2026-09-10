add_library(external.processenv STATIC)

target_sources(
    external.processenv
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

target_link_libraries(
    external.processenv
    PRIVATE
        common
)
