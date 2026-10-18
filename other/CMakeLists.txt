add_executable(
    compare
    compare.cpp
)

target_link_libraries(
    compare
    PRIVATE
        common
        lgo.io
)

install(
    TARGETS
        compare
    DESTINATION
        ${CMAKE_INSTALL_PREFIX}
)

add_executable(
    filegen
    filegen.cpp
)

target_link_libraries(
    compare
    PRIVATE
        common
)

install(
    TARGETS
        filegen
    DESTINATION
        ${CMAKE_INSTALL_PREFIX}
)
