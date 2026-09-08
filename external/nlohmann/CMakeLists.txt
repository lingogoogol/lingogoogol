add_library(external.nlohmann OBJECT)

target_sources(
    external.nlohmann
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

target_link_libraries(
    external.nlohmann
    PRIVATE
        common
)
