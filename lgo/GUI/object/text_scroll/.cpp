module lgo.GUI.object.text_scroll;

import std;

import lgo.math.vec;
import lgo.GUI.misc;
import lgo.GUI.primitive;
import lgo.GUI.font;
import lgo.GUI.object.depth_tracker;
import lgo.GUI.object.object;
import lgo.GUI.object.rect;
import lgo.GUI.object.text;
import lgo.GUI.object.click_area;
import lgo.GUI.object.std;

namespace lgo
{
    size_1D text_line_scroll_std_t::size_y{ std_forward };
    size_1D text_line_scroll_std_t::font_size{ std_font_size };
    size_1D text_line_scroll_std_t::margin{ std_margin };

    text_line_scroll_std_t::text_line_scroll_std_t
    (
        engine_t* engine,
        depth_tracker_t* depth_tracker,
        depth_range_t depth_range,
        pos_2D pos,
        std::string text,
        color_t color,
        size_1D size_x
    )
    :
        text_line_scroll_t
        {
            engine,
            depth_tracker,
            depth_range,
            pos,
            text,
            size_2D{ size_x.x, size_y.x },
            color,
            margin,
            font_preference_t::default_get(),
            color,
            alignment_2D{ alignment_x::left, alignment_y::top }
        }
    {}

    auto text_line_scroll_std_t::get_margin() const -> size_1D
    {
        return margin;
    }

    /*size_1D text_block_scroll_std_t::font_size{ std_font_size };
    size_1D text_block_scroll_std_t::margin{ std_margin };
    color_t text_block_scroll_std_t::color{ std_white };

    text_block_scroll_std_t::text_block_scroll_std_t()
    :
        text_block_scroll_std_t
        {
            nullptr,
            nullptr,
            pos_2D{ 0, 0 },
            0.0f,
            L"",
            size_2D{ 0, 0 }
        }
    {}

    text_block_scroll_std_t::text_block_scroll_std_t
    (
        engine_t* engine,
        depth_tracker_t* depth_tracker,
        pos_2D pos,
        float depth,
        std::wstring text,
        size_2D size
    )
    :
        text_block_scroll_t
        {
            engine,
            depth_tracker,
            pos,
            depth,
            text,
            size,
            font_size,
            color,
            alignment_2D{ alignment_x::left, alignment_y::top }
        }
    {}

    auto text_block_scroll_std_t::get_margin() const -> size_1D
    {
        return margin;
    }*/
}
