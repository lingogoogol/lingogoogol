export module lgo.GUI.object.menu;

import lgo.GUI.object.object;
import lgo.GUI.object.click_area;
import lgo.GUI.object.rect;
import lgo.GUI.object.text;

export namespace lgo
{
    /*class menu_t: public GUI_object_t
    {
    private:
        struct option_t
        {
            text_block_t m_text{};
            click_area_t m_click_area{};
        };
        engine_t* m_engine{};
        depth_tracker_t* m_depth_tracker{};
        std::vector<option_t> m_option{};
        rect_t m_rect{};
        pos_2D m_pos{};
        float m_depth{};
        size_2D m_option_size{};

        auto menu_mouse_move_callback(std::size_t index, pos_2D) -> void;
        auto menu_mouse_leave_callback(std::size_t index) -> void;
        auto menu_mouse_left_click_callback(std::size_t index, pos_2D) -> void;
        auto menu_mouse_left_release_callback(std::size_t index, pos_2D) -> void;
    public:
        menu_t() = default;
        
        menu_t
        (
            engine_t* engine,
            depth_tracker_t* depth_tracker,
            pos_2D pos,
            size_2D option_size,
            float depth
        );

        auto get_pos() const -> pos_2D override;
        auto set_pos(pos_2D pos) -> void override;
        auto get_size() const -> size_2D override;
        auto get_margin() const -> size_1D override;

        auto show_impl(bool base) -> void override;
        auto hide_impl(bool base) -> void override;

        auto add_option(std::wstring text, std::function<void(pos_2D)> callback) -> void;
    };*/
}
