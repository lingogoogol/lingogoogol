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
        m_advance_exact{ in.m_advance_exact },
        m_bbox_shift{ in.m_bbox_shift },
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

    auto font_char_t::advance_exact_get() const -> double
    {
        return m_advance_exact;
    }

    auto font_char_t::bbox_shift_get() const -> std::array<float, 2>
    {
        return m_bbox_shift;
    }

    auto font_char_t::init1(std::uint32_t index) -> void
    {
        FT_Face face_handle{ m_face->handle_get() };
        FT_Load_Glyph(face_handle, index, FT_LOAD_RENDER);
        // The bitmap keeps the face's raster resolution; positions and sizes are
        // logical pixels (identical to the bitmap's at raster scale 1).
        const auto scale{ static_cast<double>(m_face->raster_scale_get()) };
        m_advance.x = m_face->logical(face_handle->glyph->advance.x);
        m_advance.y = 0;
        m_advance_exact = static_cast<double>(face_handle->glyph->advance.x) / 64.0 / scale;
        //In our window/Vulkan coordinate system, (0, 0) is at the top-left and +y points downward.
        //FreeType's `bitmap_top` is the upward distance from the baseline to the top edge of the glyph bitmap,
        //so the top-left corner of the glyph bitmap relative to the baseline is `(bitmap_left, -bitmap_top)`.
        const std::int64_t left{ face_handle->glyph->bitmap_left };
        const std::int64_t top{ -face_handle->glyph->bitmap_top };
        const std::int64_t width{ face_handle->glyph->bitmap.width };
        const std::int64_t rows{ face_handle->glyph->bitmap.rows };
        // The quad covers whole logical pixels. Rounding its position and size
        // separately and stretching the bitmap into it made each glyph a slightly
        // different size and offset at non-integer scales (e.g. 2.75 on phones).
        // Instead, pad the bitmap to the quad and keep the glyph at its real
        // offset, so it is drawn unscaled to within half a framebuffer pixel.
        // At scale 1 the padding is zero and the bitmap is unchanged.
        m_bbox_pos.x = static_cast<std::int64_t>(std::floor(static_cast<double>(left) / scale));
        m_bbox_pos.y = static_cast<std::int64_t>(std::floor(static_cast<double>(top) / scale));
        m_bbox_shift = {};
        if (!width || !rows)
        {
            m_bbox_size.x = static_cast<std::int64_t>(std::ceil(static_cast<double>(width) / scale));
            m_bbox_size.y = static_cast<std::int64_t>(std::ceil(static_cast<double>(rows) / scale));
            m_pixmap.dim_set(width, rows);
            return;
        }
        m_bbox_size.x = static_cast<std::int64_t>(std::ceil(static_cast<double>(left + width) / scale)) - m_bbox_pos.x;
        m_bbox_size.y = static_cast<std::int64_t>(std::ceil(static_cast<double>(top + rows) / scale)) - m_bbox_pos.y;
        const std::int64_t pad_x{ std::llround(static_cast<double>(left) - static_cast<double>(m_bbox_pos.x) * scale) };
        const std::int64_t pad_y{ std::llround(static_cast<double>(top) - static_cast<double>(m_bbox_pos.y) * scale) };
        // Rounding the padding to whole framebuffer pixels leaves a sub-pixel error;
        // the text line shifts the texture by it.
        m_bbox_shift =
        {
            static_cast<float>(static_cast<double>(left - pad_x) / scale - static_cast<double>(m_bbox_pos.x)),
            static_cast<float>(static_cast<double>(top - pad_y) / scale - static_cast<double>(m_bbox_pos.y))
        };
        // Explicit type: std::llround returns long long, which is std::int64_t on
        // Windows but not on Android (where std::int64_t is long).
        const std::int64_t pixmap_x{ std::max<std::int64_t>(std::llround(static_cast<double>(m_bbox_size.x) * scale), pad_x + width) };
        const std::int64_t pixmap_y{ std::max<std::int64_t>(std::llround(static_cast<double>(m_bbox_size.y) * scale), pad_y + rows) };
        m_pixmap.dim_set(static_cast<pixmap_t::size_t>(pixmap_x), static_cast<pixmap_t::size_t>(pixmap_y));
        for (pixmap_t::size_t y{ 0 }; y < static_cast<pixmap_t::size_t>(pixmap_y); ++y)
        {
            for (pixmap_t::size_t x{ 0 }; x < static_cast<pixmap_t::size_t>(pixmap_x); ++x)
            {
                m_pixmap[x, y] = std::byte{};
            }
        }
        for (std::int64_t y{ 0 }; y < rows; ++y)
        {
            for (std::int64_t x{ 0 }; x < width; ++x)
            {
                m_pixmap[static_cast<pixmap_t::size_t>(pad_x + x), static_cast<pixmap_t::size_t>(pad_y + y)] = static_cast<std::byte>
                (
                    face_handle->glyph->bitmap.buffer[y * face_handle->glyph->bitmap.pitch + x]
                );
            }
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
