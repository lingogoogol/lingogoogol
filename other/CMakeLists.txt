add_executable(
    compare
    compare.cpp
)

target_link_libraries(
    compare
    PRIVATE
        common
    PUBLIC
        lgo.io
)

install(
    TARGETS
        compare
    DESTINATION
        ${CMAKE_INSTALL_PREFIX}
)
