export module lgo.GUI.object.rect;

import std;

import lgo.math.vec;
import lgo.GUI.misc;
import lgo.GUI.Vulkan;
import lgo.GUI.primitive;
import lgo.GUI.object.object;
import lgo.GUI.object.std;
import lgo.GUI.object.depth_tracker;

export namespace lgo
{
    class rect_t: public GUI_object_t
    {
    private:
        engine_t* m_engine{};
        depth_tracker_t* m_depth_tracker{};
        std::uint64_t m_depth_id{};
        pos_2D m_pos{};
        size_2D m_size{};
        depth_range_t m_depth_range{};
        color_t m_color{};
        image_view_t* m_image_view{};
        pos_2D m_texture_pos{};
        size_2D m_texture_axis_x{};
        size_2D m_texture_axis_y{};
        bool m_texture_enable{};
        rect_primitive_t* m_primitive{};
        pos_2D m_clip_pos{};
        size_2D m_clip_size{};
    public:
        rect_t() = default;

        rect_t
        (
            engine_t* engine,
            depth_tracker_t* depth_tracker,
            depth_range_t depth_range,
            pos_2D pos,
            size_2D size
        );

        rect_t
        (
            engine_t* engine,
            depth_tracker_t* depth_tracker,
            depth_range_t depth_range,
            pos_2D pos,
            size_2D size,
            pos_2D clip_pos,
            size_2D clip_size
        );

        rect_t
        (
            engine_t* engine,
            depth_tracker_t* depth_tracker,
            depth_range_t depth_range,
            pos_2D pos,
            size_2D size,
            color_t color
        );

        rect_t
        (
            engine_t* engine,
            depth_tracker_t* depth_tracker,
            depth_range_t depth_range,
            pos_2D pos,
            size_2D size,
            image_view_t* image_view
        );

        rect_t
        (
            engine_t* engine,
            depth_tracker_t* depth_tracker,
            depth_range_t depth_range,
            pos_2D pos,
            size_2D size,
            pos_2D clip_pos,
            size_2D clip_size,
            image_view_t* image_view,
            pos_2D texture_pos,
            size_2D texture_axis_x,
            size_2D texture_axis_y,
            color_t color = std_white
        );

        auto get_pos() const -> pos_2D override;
        auto set_pos(pos_2D pos) -> void override;
        auto get_size() const -> size_2D override;
        auto get_margin() const -> size_1D override;
        auto set_size(size_2D size) -> void override;
        virtual auto log(std::uint32_t tab_count) const -> void override;
        
        auto set_color(color_t color) -> void;
        auto set_image_view(image_view_t* image_view) -> void;
        auto color_enable() -> void;
        auto texture_enable() -> void;
        
        auto inside(pos_2D pos) -> bool;

        auto show_impl(bool base) -> void override;
        auto hide_impl(bool base) -> void override;
    };

    template<bool is_y>
    class std_line_impl_t: public rect_t
    {
    public:
        static size_1D width;
        static size_1D margin;
        
        std_line_impl_t();
        std_line_impl_t
        (
            engine_t* engine,
            depth_tracker_t* depth_tracker,
            depth_range_t depth_range,
            pos_2D pos,
            size_1D length,
            color_t color
        );

        auto get_margin() const -> size_1D override;
    };

    template<bool is_y>
    size_1D std_line_impl_t<is_y>::width{ std_line_size };
    template<bool is_y>
    size_1D std_line_impl_t<is_y>::margin{ std_margin };

    template<bool is_y>
    std_line_impl_t<is_y>::std_line_impl_t()
    : std_line_impl_t{ nullptr, nullptr, pos_2D{ 0, 0 }, size_1D{ 0 }, 0.0f } {}

    template<bool is_y>
    std_line_impl_t<is_y>::std_line_impl_t
    (
        engine_t* engine,
        depth_tracker_t* depth_tracker,
        depth_range_t depth_range,
        pos_2D pos,
        size_1D length,
        color_t color
    )
    :
        rect_t
        {
            engine,
            depth_tracker,
            depth_range,
            pos,
            size_2D{ is_y ? width.x : length.x, is_y ? length.x : width.x },
            color
        }
    {}

    template<bool is_y>
    auto std_line_impl_t<is_y>::get_margin() const -> size_1D
    {
        return margin;
    }

    using std_linex_t = std_line_impl_t<false>;
    using std_liney_t = std_line_impl_t<true>;
}
