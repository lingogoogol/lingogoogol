export module lgo.GUI.object.rect_border;

import std;

import lgo.math.vec;
import lgo.GUI.misc;
import lgo.GUI.primitive;
import lgo.GUI.object.object;
import lgo.GUI.object.std;
import lgo.GUI.object.depth_tracker;
import lgo.GUI.object.rect;

export namespace lgo
{
    class rect_border_t: public GUI_object_t
    {
    private:
        size_1D m_border_size{};
        depth_range_t m_depth_range{};
        rect_t m_outer_rect{};
        rect_t m_inner_rect{};

        auto compute_inner_pos() -> pos_2D;
        auto compute_inner_size() -> size_2D;
    public:
        static size_1D margin;

        rect_border_t() = default;

        rect_border_t
        (
            engine_t* engine,
            depth_tracker_t* depth_tracker,
            pos_2D pos,
            size_2D size,
            color_t content_color,
            size_1D border_size,
            color_t border_color,
            depth_range_t depth_range
        );

        auto get_pos() const -> pos_2D override;
        auto set_pos(pos_2D pos) -> void override;
        auto get_size() const -> size_2D override;
        auto set_size(size_2D size) -> void override;
        auto get_border_size() const -> size_1D;
        auto get_margin() const -> size_1D override;
        auto depth_range_get() const -> depth_range_t;
        virtual auto log(std::uint32_t tab_count) const -> void override;
        
        auto border_set_color(color_t color) -> void;
        auto content_get_pos() -> pos_2D;
        auto content_get_size() -> size_2D;
        auto content_set_color(color_t color) -> void;

        auto inside(pos_2D pos) -> bool;

        auto show_impl(bool base) -> void override;
        auto hide_impl(bool base) -> void override;
    };

    class std_border_t: public rect_border_t
    {
    public:
        static color_t content_color;
        static size_1D border_size;
        static size_1D margin;

        std_border_t();

        std_border_t
        (
            engine_t* engine,
            depth_tracker_t* depth_tracker,
            pos_2D pos,
            size_2D size,
            color_t border_color,
            depth_range_t depth_range
        );
    };
}
