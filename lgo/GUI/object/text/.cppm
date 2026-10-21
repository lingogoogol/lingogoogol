export module lgo.GUI.object.text;

import std;

import lgo.math.vec;
import lgo.GUI.misc;
import lgo.GUI.Vulkan;
import lgo.GUI.primitive;
import lgo.GUI.font;
import lgo.GUI.object.object;
import lgo.GUI.object.depth_tracker;
import lgo.GUI.object.std;
import lgo.GUI.object.virt;
import lgo.GUI.object.div;

export namespace lgo
{
    class text_line_t: public GUI_object_t
    {
    public:
        struct ctor_t
        {
            engine_t* engine{};
            depth_tracker_t* depth_tracker{};
            depth_range_t depth_range{};
            pos_2D pos{};
            std::string text{};
            size_2D size{};
            size_1D margin{};
            font_preference_t* font_preference{};
            color_t color{};
            alignment_2D alignment{};
        };

        text_line_t() = default;

        text_line_t
        (
            engine_t* engine,
            depth_tracker_t* depth_tracker,
            depth_range_t depth_range,
            pos_2D pos,
            std::string text,
            size_2D size,
            size_1D margin,
            const font_preference_t& font_preference,
            color_t color,
            alignment_2D alignment,
            bool load_now = true
        );

        auto init
        (
            engine_t* engine,
            depth_tracker_t* depth_tracker,
            depth_range_t depth_range,
            pos_2D pos,
            std::string text,
            size_2D size,
            size_1D margin,
            const font_preference_t& font_preference,
            color_t color,
            alignment_2D alignment,
            bool load_now = true
        )
        -> void;

        virtual ~text_line_t();

        auto get_pos() const -> pos_2D override;
        auto set_pos(pos_2D pos) -> void override;
        auto get_size() const -> size_2D override;
        auto set_size(size_2D size) -> void;
        auto get_margin() const -> size_1D override;
        auto depth_range_get() const -> depth_range_t;
        virtual auto log(std::uint32_t tab_count) const -> void override;

        auto get_text() const -> const std::string&;
        auto set_text(std::string text, bool load_now = true) -> void;
        auto get_clip_pos() const -> pos_2D;
        auto set_clip_pos(pos_2D clip_pos, bool load_now = true) -> void;
        auto get_clip_size() const -> size_2D;
        auto set_clip_size(size_2D clip_size, bool load_now = true) -> void;
        auto set_color(color_t color) -> void;
        auto set_frame_color(color_t) -> void;
        auto get_text_size() const -> size_2D;

        auto show_impl(bool base) -> void override;
        auto hide_impl(bool base) -> void override;

        //Always lock m_mutex before m_load_mutex to prevent deadlock.
        static std::mutex m_load_mutex;
        static std::set<text_line_t*> m_pending;

        static auto pending_load() -> void;
        static auto free() -> void;
    protected:
        engine_t* m_engine{};
    private:
        struct glyph_t
        {
            font_char_t m_font{};
            image_t m_image{};
            std::uint64_t m_index{};
            image_view_t m_image_view{};
        };

        static std::map<const font_face_t*, std::map<char32_t, glyph_t>> m_font;
        static std::uint64_t m_font_heap_index;

        //Populate m_div and m_glyph based on other members.
        auto load() -> void;
        auto load_try(bool load_now) -> void;

        const font_preference_t* m_font_preference{};
        std::string m_text{};
        std::vector<glyph_t*> m_glyph{};
        divx_soft_t m_div{};
        depth_range_t m_depth_range{};
        pos_2D m_clip_pos{};
        size_2D m_clip_size{};
        size_1D m_margin{};
        color_t m_color{};
        bool m_load_pending{};
    };

    class text_line_std_t: public text_line_t
    {
    public:
        static size_1D margin;

        text_line_std_t() = default;
        
        text_line_std_t
        (
            engine_t* engine,
            depth_tracker_t* depth_tracker,
            depth_range_t depth_range,
            pos_2D pos,
            std::string text,
            color_t color,
            size_1D size_x
        );
    };

    class text_line_flex_t: public text_line_t
    {
    private:
        size_1D m_size_x_min{};
    public:
        text_line_flex_t() = default;

        text_line_flex_t
        (
            engine_t* engine,
            depth_tracker_t* depth_tracker,
            depth_range_t depth_range,
            pos_2D pos,
            std::string text,
            size_1D size_x_min,
            size_1D size_y,
            size_1D margin,
            const font_preference_t& font_preference,
            color_t color,
            alignment_2D alignment
        );

        auto set_text(std::string text, bool load_now = true) -> void;
    };

    class text_line_flex_std_t: public text_line_flex_t
    {
    public:
        static size_1D margin;

        text_line_flex_std_t() = default;

        text_line_flex_std_t
        (
            engine_t* engine,
            depth_tracker_t* depth_tracker,
            depth_range_t depth_range,
            pos_2D pos,
            std::string text,
            size_1D size_x_min,
            color_t color
        );
    };
}
