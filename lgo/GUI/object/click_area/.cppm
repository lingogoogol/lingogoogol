export module lgo.GUI.object.click_area;

import std;

import lgo.math.vec;
import lgo.GUI.misc;
import lgo.GUI.primitive;
import lgo.GUI.object.object;
import lgo.GUI.object.depth_tracker;

export namespace lgo
{
    class click_area_t: public GUI_object_t
    {
    public:
        using callback_mouse_leave_t = std::function<void(void)>;
        using callback_mouse_move_t = std::function<void(pos_2D)>;
        using callback_mouse_left_click_t = std::function<void(pos_2D)>;
        using callback_mouse_left_release_t = std::function<void(pos_2D)>;
        using callback_t = std::function<void(void)>;
    private:
        engine_t* m_engine{};
        depth_tracker_t* m_depth_tracker{};
        std::uint64_t m_depth_id{};
        pos_2D m_pos{};
        size_2D m_size{};
        float m_depth{};
        callback_mouse_leave_t m_callback_mouse_leave{};
        callback_mouse_move_t m_callback_mouse_move{};
        callback_mouse_left_click_t m_callback_mouse_left_click{};
        callback_mouse_left_release_t m_callback_mouse_left_release{};
        callback_t m_callback{};
        engine_t::callback_mouse_enter_t* m_callback_mouse_enter_key{};
        engine_t::callback_mouse_move_t* m_callback_mouse_move_key{};
        engine_t::callback_mouse_button_t* m_callback_mouse_button_key{};
        std::atomic_bool m_inside{ false };
        std::atomic_bool m_clicked_inside{ false };
        
        auto clip_changed() -> void override;
        auto click_area_callback_mouse_enter(int entered) -> void;
        auto click_area_callback_mouse_move(pos_2D pos) -> void;
        auto click_area_callback_mouse_button(int button, int action, int mods) -> void;
    public:
        click_area_t() = default;
        click_area_t(const click_area_t& in);
        click_area_t
        (
            engine_t* engine,
            depth_tracker_t* depth_tracker,
            depth_range_t depth_range,
            pos_2D pos,
            size_2D size,
            callback_mouse_leave_t callback_mouse_leave,
            callback_mouse_move_t callback_mouse_move,
            callback_mouse_left_click_t callback_mouse_left_click,
            callback_mouse_left_release_t callback_mouse_left_release,
            callback_t callback
        );
        auto operator=(const click_area_t& in) -> click_area_t&;

        auto get_pos() const -> pos_2D override;
        auto set_pos(pos_2D pos) -> void override;
        auto get_size() const -> size_2D override;
        auto set_size(size_2D size) -> void override;
        auto get_margin() const -> size_1D override;
        virtual auto log(std::uint32_t tab_count) const -> void override;

        auto show_impl(bool base) -> void override;
        auto hide_impl(bool base) -> void override;
    };
}
