module lgo.GUI.font.cha;

import std;

import external.FreeType;

import lgo.math.vec;
import lgo.GUI.font.face;
import lgo.GUI.font.preference;

namespace lgo
{
    font_char_t::font_char_t(char32_t code)
    : font_char_t{ code, &font_preference_t::default_get() }
    {}

    font_char_t::font_char_t(char32_t code, font_preference_t* face_preference)
    : m_code{ code }
    {
        init1(face_preference);
        return;
    }

    font_char_t::font_char_t(font_char_t&& in)
    :
        m_pixmap{ std::move(in.m_pixmap) },
        m_bbox_pos{ in.m_bbox_pos },
        m_bbox_size{ in.m_bbox_size },
        m_advance{ in.m_advance },
        m_code{ in.m_code },
        m_face{ in.m_face }
    {}

    auto font_char_t::init(char32_t code) -> void
    {
        init(code, &font_preference_t::default_get());
        return;
    }

    auto font_char_t::init(char32_t code, font_face_t* face, std::uint32_t index) -> void
    {
        m_code = code;
        m_face = face;
        init1(index);
        return;
    }

    auto font_char_t::init(char32_t code, font_preference_t* face_preference) -> void
    {
        m_code = code;
        init1(face_preference);
        return;
    }

    auto font_char_t::pixmap_get() const -> const pixmap_t&
    {
        return m_pixmap;
    }

    auto font_char_t::advance_get() const -> pos_2D
    {
        return m_advance;
    }

    auto font_char_t::bbox_pos_get() const -> size_2D
    {
        return m_bbox_pos;
    }

    auto font_char_t::bbox_size_get() const -> size_2D
    {
        return m_bbox_size;
    }

    auto font_char_t::init1(std::uint32_t index) -> void
    {
        FT_Face face_handle{ m_face->handle_get() };
        FT_Load_Glyph(face_handle, index, FT_LOAD_RENDER);
        m_advance.x = face_handle->glyph->advance.x >> 6;
        m_advance.y = 0;
        m_bbox_pos.x = face_handle->glyph->bitmap_left;
        m_bbox_pos.y = face_handle->glyph->bitmap_top - face_handle->glyph->bitmap.rows;
        m_bbox_size.x = face_handle->glyph->bitmap.width;
        m_bbox_size.y = face_handle->glyph->bitmap.rows;
        m_pixmap.dim_set(face_handle->glyph->bitmap.width, face_handle->glyph->bitmap.rows);
        for (pixmap_t::size_t xy{ 0 }; xy < m_pixmap.pixel_count(); ++xy)
        {
            m_pixmap[xy] = static_cast<std::byte>(face_handle->glyph->bitmap.buffer[xy]);
        }
        return;
    }

    auto font_char_t::init1(font_preference_t* face_preference) -> void
    {
        std::uint32_t index{};
        m_face = face_preference->favorite(m_code, &index);
        init1(index);
        return;
    }
}
