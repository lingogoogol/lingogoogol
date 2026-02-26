#ifndef COMPILERCPP_LIB_GUI_BUTTON_DEF
#define COMPILERCPP_LIB_GUI_BUTTON_DEF

#include <functional>

#include "object.hpp"
#include "stu.hpp"
#include "engine_decl.hpp"
#include "rect_primitive_def.hpp"
#include "text_primitive_def.hpp"

class button_t: public GUI_object {
private:
    engine_t* m_engine{};
    pos_2D m_pos{};
    size_2D m_size{};
    size_1D m_border_size{};
    std::wstring m_display_text{};
    depth_range_t m_depth_range{};
    rect_primitive_t* m_outer_rect{};
    rect_primitive_t* m_inner_rect{};
    text_primitive_t* m_text{};
    std::function<void(pos_2D)>* m_mouse_move_callback{};
    std::function<void(pos_2D)>* m_mouse_left_click_callback{};
    std::function<void(pos_2D)>* m_mouse_left_release_callback{};
    std::function<void(void)> m_callback{};
    bool m_clicked_inside{};
    
    auto mouse_move_callback(pos_2D pos) -> void;
    auto mouse_left_click_callback(pos_2D pos) -> void;
    auto mouse_left_release_callback(pos_2D pos) -> void;
public:
    button_t() = default;
    button_t(engine_t* engine, pos_2D pos, size_2D size, size_1D border_size
    , std::wstring display_text, depth_range_t depth_range, std::function<void(void)> callback);

    auto show() -> void override;
    auto hide() -> void override;
};

#endif