#ifndef COMPILERCPP_GUI_FREETYPE_FONT_CHAR
#define COMPILERCPP_GUI_FREETYPE_FONT_CHAR

#include <vector>

#include "font_preference.hpp"

#include "../primitive/engine_def.hpp"
#include "../directx/pixmap.hpp"

class font_char_t {
public:
    font_char_t() = default;
    font_char_t(char32_t code);
    font_char_t(char32_t code, font_preference_t* face_preference);
    font_char_t(font_char_t&& in);
    auto init(char32_t code) -> void;
    auto init(char32_t code, font_face_t* face, std::uint32_t index) -> void;
    auto init(char32_t code, font_preference_t* face_preference) -> void;

    auto pixmap_get() const -> const pixmap_t&;
    auto advance_get() const -> pos_2D;
    auto bbox_pos_get() const -> size_2D;
    auto bbox_size_get() const -> size_2D;
private:
    //Load data without modifying m_code and m_face.
    auto init1(std::uint32_t index) -> void;
    //Load data without modifying m_code.
    auto init1(font_preference_t* face_preference) -> void;

    pixmap_t m_pixmap{};
    size_2D m_bbox_pos{};
    size_2D m_bbox_size{};
    pos_2D m_advance{};
    char32_t m_code{};
    font_face_t* m_face{};
};

font_char_t::font_char_t(char32_t code): font_char_t{ code, &font_preference_t::default_get() } {}

font_char_t::font_char_t(char32_t code, font_preference_t* face_preference)
: m_code{ code } {
    init1(face_preference);
    return;
}

font_char_t::font_char_t(font_char_t&& in): m_pixmap{ std::move(in.m_pixmap) }, m_bbox_pos{ in.m_bbox_pos }
, m_bbox_size{ in.m_bbox_size }, m_advance{ in.m_advance }, m_code{ in.m_code }, m_face{ in.m_face } {}

auto font_char_t::init(char32_t code) -> void {
    init(code, &font_preference_t::default_get());
    return;
}

auto font_char_t::init(char32_t code, font_face_t* face, std::uint32_t index) -> void {
    m_code = code;
    m_face = face;
    init1(index);
    return;
}

auto font_char_t::init(char32_t code, font_preference_t* face_preference) -> void {
    m_code = code;
    init1(face_preference);
    return;
}

auto font_char_t::pixmap_get() const -> const pixmap_t& {
    return m_pixmap;
}

auto font_char_t::advance_get() const -> pos_2D {
    return m_advance;
}

auto font_char_t::bbox_pos_get() const -> size_2D {
    return m_bbox_pos;
}

auto font_char_t::bbox_size_get() const -> size_2D {
    return m_bbox_size;
}

auto font_char_t::init1(std::uint32_t index) -> void {
    FT_Face face_handle{ m_face->handle_get() };
    FT_Load_Glyph(face_handle, index, FT_LOAD_RENDER);
    m_advance.x = face_handle->glyph->advance.x;
    m_advance.y = 0;
    m_bbox_pos.x = face_handle->glyph->bitmap_left;
    m_bbox_pos.y = face_handle->glyph->bitmap_top - face_handle->glyph->bitmap.rows;
    m_bbox_size.x = face_handle->glyph->bitmap.width;
    m_bbox_size.y = face_handle->glyph->bitmap.rows;
    m_pixmap.size_set(face_handle->glyph->bitmap.width, face_handle->glyph->bitmap.rows);
    for (pixmap_t::size_t xy{ 0 }; xy < m_pixmap.size_1D_get(); ++xy) {
        m_pixmap[xy] = static_cast<std::byte>(face_handle->glyph->bitmap.buffer[xy]);
    }
    return;
}

auto font_char_t::init1(font_preference_t* face_preference) -> void {
    std::uint32_t index{};
    m_face = face_preference->favorite(m_code, &index);
    init1(index);
    return;
}

#endif