add_library(lgo.GUI.object STATIC)

target_sources(
    lgo.GUI.object
    PUBLIC
        FILE_SET CXX_MODULES
        FILES
            .cppm
)

add_subdirectory(state)
add_subdirectory(depth_tracker)
add_subdirectory(object)
add_subdirectory(std)
add_subdirectory(div)
add_subdirectory(div_border)
add_subdirectory(rect)
add_subdirectory(rect_border)
add_subdirectory(text)
add_subdirectory(text_scroll)
add_subdirectory(text_input)
add_subdirectory(click_area)
add_subdirectory(button)
add_subdirectory(plus)
add_subdirectory(fun)
add_subdirectory(focus)
add_subdirectory(menu)
add_subdirectory(virt)
add_subdirectory(dialog)

target_link_libraries(
    lgo.GUI.object
    PRIVATE
        lgo.GUI.object.dialog
        common
        lgo.GUI.object.state
        lgo.GUI.object.depth_tracker
        lgo.GUI.object.object
        lgo.GUI.object.std
        lgo.GUI.object.div
        lgo.GUI.object.div_border
        lgo.GUI.object.rect
        lgo.GUI.object.rect_border
        lgo.GUI.object.text
        lgo.GUI.object.text_scroll
        lgo.GUI.object.text_input
        lgo.GUI.object.click_area
        lgo.GUI.object.button
        lgo.GUI.object.plus
        lgo.GUI.object.fun
        lgo.GUI.object.focus
        lgo.GUI.object.menu
        lgo.GUI.object.virt
)
