export module lgo.GUI.object.plus;

import std;

import lgo.math.vec;
import lgo.GUI.misc;
import lgo.GUI.primitive;
import lgo.GUI.object.object;
import lgo.GUI.object.click_area;
import lgo.GUI.object.rect;
import lgo.GUI.object.rect_border;
import lgo.GUI.object.text;
import lgo.GUI.object.depth_tracker;

export namespace lgo
{
    class button_plus_t: public GUI_object_t
    {
    private:
        engine_t* m_engine{};
        click_area_t m_click_area{};
        rect_border_t m_rect{};
        rect_t m_horizontal{}, m_vertical{};
        color_t m_border_color{};
        
        auto button_plus_callback_mouse_leave() -> void;
        auto button_plus_callback_mouse_move(pos_2D) -> void;
        auto button_plus_callback_mouse_left_click(pos_2D) -> void;
        auto button_plus_callback_mouse_left_release(pos_2D) -> void;
    public:
        button_plus_t() = default;

        button_plus_t
        (
            engine_t* engine,
            depth_tracker_t* depth_tracker,
            pos_2D pos,
            size_2D size,
            color_t content_color,
            size_1D border_size,
            color_t border_color,
            depth_range_t depth_range,
            std::function<void(void)> callback
        );

        auto get_pos() const -> pos_2D override;
        auto set_pos(pos_2D pos) -> void override;
        auto get_size() const -> size_2D override;
        auto get_margin() const -> size_1D override;

        auto show_impl(bool base) -> void override;
        auto hide_impl(bool base) -> void override;
    };

    class std_button_plus_t: public button_plus_t
    {
    public:
        static size_2D size;
        static color_t content_color;
        static size_1D border_size;
        static size_1D margin;

        std_button_plus_t();

        std_button_plus_t
        (
            engine_t* engine,
            depth_tracker_t* depth_tracker,
            pos_2D pos,
            depth_range_t depth_range,
            color_t color,
            std::function<void(void)> callback
        );

        auto get_margin() const -> size_1D override;
    };
}
