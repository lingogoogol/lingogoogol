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
        bin
)

add_executable(
    filegen
    filegen.cpp
)

target_link_libraries(
    filegen
    PRIVATE
        common
)

install(
    TARGETS
        filegen
    DESTINATION
        bin
)
