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
    private:
        engine_t* m_engine{};
        depth_tracker_t* m_depth_tracker{};
        std::uint64_t m_depth_id{};
        pos_2D m_pos{};
        size_2D m_size{};
        float m_depth{};
        std::function<void(pos_2D)> m_mouse_move_callback{};
        std::function<void(void)> m_mouse_leave_callback{};
        std::function<void(pos_2D)> m_mouse_left_click_callback{};
        std::function<void(pos_2D)> m_mouse_left_release_callback{};
        std::function<void(void)> m_callback{};
        std::function<void(pos_2D)>* m_mouse_move_callback_key{};
        std::function<void(void)>* m_mouse_leave_callback_key{};
        std::function<void(pos_2D)>* m_mouse_left_click_callback_key{};
        std::function<void(pos_2D)>* m_mouse_left_release_callback_key{};
        std::atomic_bool m_inside{ false };
        std::atomic_bool m_clicked_inside{ false };
        
        auto click_area_mouse_move_callback(pos_2D pos) -> void;
        auto click_area_mouse_leave_callback() -> void;
        auto click_area_mouse_left_click_callback(pos_2D pos) -> void;
        auto click_area_mouse_left_release_callback(pos_2D pos) -> void;
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
            std::function<void(pos_2D)> mouse_move_callback,
            std::function<void(void)> mouse_leave_callback,
            std::function<void(pos_2D)> mouse_left_click_callback,
            std::function<void(pos_2D)> mouse_left_release_callback,
            std::function<void(void)> callback
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
