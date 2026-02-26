#ifndef COMPILERCPP_GUI_OBJECT_TEXT
#define COMPILERCPP_GUI_OBJECT_TEXT

#include <string>
#include <map>
#include <vector>

#include "object_def.hpp"
#include "std.hpp"
#include "virt.hpp"

#include "../../lib/stu.hpp"
#include "../primitive/engine_def.hpp"

//engine_t* engine,
//depth_tracker_t* depth_tracker,
//depth_range_t depth_range,
//pos_2D pos,
//std::string text,
//size_2D size,
//size_1D margin,
//const font_preference_t& font_preference,
//color_t color,
//alignment_2D alignment
class text_line_t: public GUI_object_t {
public:
    static auto init_global(Microsoft::WRL::ComPtr<ID3D12Device> device) -> void;

    text_line_t() = default;
    text_line_t(
        engine_t* engine,
        depth_tracker_t* depth_tracker,
        depth_range_t depth_range,
        pos_2D pos,
        std::string text,
        size_2D size,
        size_1D margin,
        const font_preference_t& font_preference,
        color_t color,
        alignment_2D alignment
    );
    auto init(
        engine_t* engine,
        depth_tracker_t* depth_tracker,
        depth_range_t depth_range,
        pos_2D pos,
        std::string text,
        size_2D size,
        size_1D margin,
        const font_preference_t& font_preference,
        color_t color,
        alignment_2D alignment
    );

    auto get_pos() const -> pos_2D override;
    auto set_pos(pos_2D pos) -> void override;
    auto get_size() const -> size_2D override;
    auto get_margin() const -> size_1D override;
    auto depth_range_get() const -> depth_range_t;
    auto get_text() const -> const std::string&;
    auto set_text(std::string text) -> void;
    auto get_clip_pos() const -> pos_2D;
    auto set_clip_pos(pos_2D clip_pos) -> void;
    auto get_clip_size() const -> size_2D;
    auto set_clip_size(size_2D clip_size) -> void;
    auto set_color(color_t color) -> void;
    auto set_frame_color(color_t) -> void;
    auto get_text_size() const -> size_2D;

    auto show_impl(bool base) -> void override;
    auto hide_impl(bool base) -> void override;
protected:
    engine_t* m_engine{};
private:
    struct glyph_t {
        font_char_t m_font{};
        texture_t m_texture{};
        std::uint64_t m_index{};
        SRV_t m_SRV{};
    };

    static descriptor_heap_t m_font_heap;
    static std::map<const font_face_t*, std::map<char32_t, glyph_t>> m_font;
    static std::uint64_t m_font_heap_index;

    //Populate m_div and m_glyph based on other members.
    auto init1() -> void;

    const font_preference_t* m_font_preference{};
    std::string m_text{};
    std::vector<glyph_t*> m_glyph{};
    divx_soft_t m_div{};
    depth_range_t m_depth_range{};
    pos_2D m_clip_pos{};
    size_2D m_clip_size{};
    size_1D m_margin{};
    color_t m_color{};
};

descriptor_heap_t text_line_t::m_font_heap{};
std::map<const font_face_t*, std::map<char32_t, text_line_t::glyph_t>> text_line_t::m_font{};
std::uint64_t text_line_t::m_font_heap_index{ 0 };

auto text_line_t::init_global(Microsoft::WRL::ComPtr<ID3D12Device> device) -> void {
    m_font_heap.init(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 0x1000, "text_line_t::m_font_heap");
    return;
}

text_line_t::text_line_t(
    engine_t* engine,
    depth_tracker_t* depth_tracker,
    depth_range_t depth_range,
    pos_2D pos,
    std::string text,
    size_2D size,
    size_1D margin,
    const font_preference_t& font_preference,
    color_t color,
    alignment_2D alignment
):
    m_engine{ engine },
    m_div{ engine, depth_tracker, depth_range, pos, size, size_1D{ 0 }, alignment, false },
    m_depth_range{ depth_range },
    m_text{ text },
    m_font_preference{ &font_preference },
    m_margin{ margin },
    m_color{ color }
{
    init1();
    return;
}

auto text_line_t::init(
    engine_t* engine,
    depth_tracker_t* depth_tracker,
    depth_range_t depth_range,
    pos_2D pos,
    std::string text,
    size_2D size,
    size_1D margin,
    const font_preference_t& font_preference,
    color_t color,
    alignment_2D alignment
) {
    m_engine = engine;
    m_div.init(engine, depth_tracker, depth_range, pos, size, size_1D{ 0 }, alignment, false);
    m_depth_range = depth_range;
    m_text = text;
    m_font_preference = &font_preference;
    m_color = color;
    return;
}

auto text_line_t::get_pos() const -> pos_2D {
    std::unique_lock lock{ m_mutex };
    return m_div.get_pos();
}

auto text_line_t::set_pos(pos_2D pos) -> void {
    std::unique_lock lock{ m_mutex };
    m_div.set_pos(pos);
    return;
}

auto text_line_t::get_size() const -> size_2D {
    std::unique_lock lock{ m_mutex };
    return m_div.get_size();
}

auto text_line_t::get_margin() const -> size_1D {
    return m_margin;
}

auto text_line_t::depth_range_get() const -> depth_range_t {
    std::unique_lock lock{ m_mutex };
    return m_depth_range;
}

auto text_line_t::get_text() const -> const std::string& {
    std::unique_lock lock{ m_mutex };
    return m_text;
}

auto text_line_t::set_text(std::string text) -> void {
    std::unique_lock lock{ m_mutex };
    m_text = text;
    init1();
    return;
}

auto text_line_t::get_clip_pos() const -> pos_2D {
    std::unique_lock lock{ m_mutex };
    return m_clip_pos;
}

auto text_line_t::set_clip_pos(pos_2D clip_pos) -> void {
    std::unique_lock lock{ m_mutex };
    m_clip_pos = clip_pos;
    init1();
    return;
}

auto text_line_t::get_clip_size() const -> size_2D {
    std::unique_lock lock{ m_mutex };
    return m_clip_size;
}

auto text_line_t::set_clip_size(size_2D clip_size) -> void {
    std::unique_lock lock{ m_mutex };
    m_clip_size = clip_size;
    init1();
    return;
}

auto text_line_t::set_color(color_t color) -> void {
    m_color = color;
    //...
    return;
}

auto text_line_t::set_frame_color(color_t) -> void {}

auto text_line_t::get_text_size() const -> size_2D {
    std::unique_lock lock{ m_mutex };
    return size_2D{ m_div.len().x, (*m_font_preference)[0]->leading_get() };
}

auto text_line_t::show_impl(bool base) -> void {
    if (!show_begin(base)) { return; }
    std::unique_lock lock{ m_mutex };
    m_div.show();
    show_end(base);
    return;
}

auto text_line_t::hide_impl(bool base) -> void {
    if (!hide_begin(base)) { return; }
    std::unique_lock lock{ m_mutex };
    m_div.hide();
    hide_end(base);
    return;
}

auto text_line_t::init1() -> void {
    m_div.clear_state();
    std::wstring text_w{ to_wstring(m_text) };
    command_queue_t& command_queue{ m_engine->command_queue_get() };
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list{ command_queue.create_list() };
    m_glyph.resize(text_w.size());
    for (std::size_t i{ 0 }; i < text_w.size(); ++i) {
        std::uint32_t index{};
        font_face_t* face{ m_font_preference->favorite(text_w[i], &index) };
        if (!m_font.contains(face)) {
            m_font.emplace(face, std::map<char32_t, glyph_t>{});
        }
        std::map<char32_t, glyph_t>& face_loaded{ m_font[face] };
        auto find_result{ face_loaded.find(text_w[i]) };
        if (find_result == face_loaded.end()) {
            glyph_t glyph{};
            glyph.m_font.init(text_w[i], face, index);
            m_engine->log_info_queue();
            glyph.m_texture.init(m_engine->device_get(), command_list, glyph.m_font.pixmap_get()
            , "text_line_t::m_font[family = " + face->family_get() + ", style = " + face->style_get() + "][code = "
            + std::to_string(static_cast<std::uint32_t>(text_w[i])) + "].m_texture");
            m_engine->log_info_queue();
            glyph.m_index = m_font_heap_index;
            glyph.m_SRV = m_font_heap.create_SRV(m_engine->device_get(), glyph.m_texture, m_font_heap_index);
            ++m_font_heap_index;
            m_glyph[i] = &face_loaded.emplace(text_w[i], std::move(glyph)).first->second;
        }
        else {
            m_glyph[i] = &find_result->second;
        }
        m_div.add_object<rect_t>(
            m_depth_range,
            size_2D{ m_glyph[i]->m_font.advance_get().x, face->height_get() },
            m_clip_pos,
            m_clip_size,
            m_glyph[i]->m_SRV,
            pos_2D{ 0, -face->descender_get() } + m_glyph[i]->m_font.bbox_pos_get(),
            size_2D{ m_glyph[i]->m_font.bbox_size_get().x, 0 },
            size_2D{ 0, m_glyph[i]->m_font.bbox_size_get().y }
        );
    }
    command_queue.execute_list(command_list);
    return;
}

//engine_t* engine,
//depth_tracker_t* depth_tracker,
//depth_range_t depth_range,
//pos_2D pos,
//std::string text,
//color_t color,
//size_1D size_x
class text_line_std_t: public text_line_t {
public:
    static size_1D margin;

    text_line_std_t() = default;
    text_line_std_t(
        engine_t* engine,
        depth_tracker_t* depth_tracker,
        depth_range_t depth_range,
        pos_2D pos,
        std::string text,
        color_t color,
        size_1D size_x
    );
};

size_1D text_line_std_t::margin{ std_margin };

text_line_std_t::text_line_std_t(
    engine_t* engine,
    depth_tracker_t* depth_tracker,
    depth_range_t depth_range,
    pos_2D pos,
    std::string text,
    color_t color,
    size_1D size_x
):
    text_line_t{
        engine,
        depth_tracker,
        depth_range,
        pos,
        text,
        size_2D{ size_x.x, font_preference_t::default_get()[0]->leading_get() },
        margin,
        font_preference_t::default_get(),
        color,
        alignment_2D{ alignment_x::left, alignment_y::center }
    }
{}

//engine_t* engine,
//depth_tracker_t* depth_tracker,
//depth_range_t depth_range,
//pos_2D pos,
//std::string text,
//size_1D size_x_min,
//size_1D size_y,
//size_1D margin,
//font_preference_t* font_preference,
//color_t color,
//alignment_2D alignment
class text_line_flex_t: public text_line_t {
private:
    size_1D m_size_x_min{};
public:
    text_line_flex_t() = default;
    text_line_flex_t(
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

    auto set_text(std::string text) -> void;
};

text_line_flex_t::text_line_flex_t(
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
):
    text_line_t{
        engine,
        depth_tracker,
        depth_range,
        pos,
        "",
        size_2D{ 0, size_y.x },
        margin,
        font_preference,
        color,
        alignment
    },
    m_size_x_min{ size_x_min }
{
    set_text(text);
    return;
}

auto text_line_flex_t::set_text(std::string text) -> void {
    std::unique_lock lock{ m_mutex };
    text_line_t::set_text(text);
    size_1D size_x{ std::max(get_text_size().x, m_size_x_min.x) };
    text_line_t::set_size(size_2D{ size_x.x, get_size().y });
    text_line_t::set_clip_size(size_2D{ size_x.x, get_clip_size().y });
    return;
}

//engine_t* engine,
//depth_tracker_t* depth_tracker,
//depth_range_t depth_range,
//pos_2D pos,
//std::string text,
//size_1D size_x_min,
//color_t color
class text_line_flex_std_t: public text_line_flex_t {
public:
    static size_1D margin;

    text_line_flex_std_t() = default;
    text_line_flex_std_t(
        engine_t* engine,
        depth_tracker_t* depth_tracker,
        depth_range_t depth_range,
        pos_2D pos,
        std::string text,
        size_1D size_x_min,
        color_t color
    );
};

size_1D text_line_flex_std_t::margin{ std_margin };

text_line_flex_std_t::text_line_flex_std_t(
    engine_t* engine,
    depth_tracker_t* depth_tracker,
    depth_range_t depth_range,
    pos_2D pos,
    std::string text,
    size_1D size_x_min,
    color_t color
):
    text_line_flex_t{
        engine,
        depth_tracker,
        depth_range,
        pos,
        text,
        size_x_min,
        std_forward,
        margin,
        font_preference_t::default_get(),
        color,
        alignment_2D{ alignment_x::left, alignment_y::center }
    }
{}

#endif