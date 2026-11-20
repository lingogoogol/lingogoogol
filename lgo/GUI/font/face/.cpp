module lgo.GUI.font.face;

import std;

import external.FreeType;

import lgo.io.file;
import lgo.GUI.font.FreeType;

namespace lgo
{
    FreeType_t* font_face_t::m_freetype{};
    float font_face_t::m_raster_scale_default{ 1 };

    auto font_face_t::raster_scale_set(float scale) -> void
    {
        m_raster_scale_default = scale > 0 ? scale : 1;
    }

    auto font_face_t::raster_scale_get() const -> float
    {
        return m_raster_scale;
    }

    auto font_face_t::logical(std::int64_t length) const -> std::int64_t
    {
        return static_cast<std::int64_t>(std::floor(static_cast<double>(length) / 64.0 / m_raster_scale));
    }

    auto font_face_t::freetype_set(FreeType_t* freetype) -> void
    {
        m_freetype = freetype;
        return;
    }

    font_face_t::font_face_t(std::string path, std::uint64_t pixel_size)
    : m_raster_scale{ m_raster_scale_default }
    {
        FT_New_Face
        (
            m_freetype->handle_get(),
            path.data(),
            0,
            &m_handle
        );
        FT_Set_Pixel_Sizes
        (
            m_handle,
            static_cast<FT_UInt>(std::lround(static_cast<double>(pixel_size) * m_raster_scale)),
            static_cast<FT_UInt>(std::lround(static_cast<double>(pixel_size) * m_raster_scale))
        );
        return;
    }

    font_face_t::~font_face_t()
    {
        FT_Done_Face(m_handle);
        return;
    }

    auto font_face_t::handle_get() const -> FT_Face
    {
        return m_handle;
    }

    //Use `m_handle->size->metrics` (26.6 fixed-point pixels for the active `pixel_size`, shifted right by 6)
    //instead of `m_handle->height` / `ascender` / `descender` directly on `FT_FaceRec`, which are in unscaled
    //font design units (e.g. 1000 or 2048 units per EM) regardless of `FT_Set_Pixel_Sizes`.
    auto font_face_t::leading_get() const -> std::int16_t
    {
        return static_cast<std::int16_t>(logical(m_handle->size->metrics.height));
    }

    auto font_face_t::ascender_get() const -> std::int16_t
    {
        return static_cast<std::int16_t>(logical(m_handle->size->metrics.ascender));
    }

    auto font_face_t::descender_get() const -> std::int16_t
    {
        return static_cast<std::int16_t>(logical(m_handle->size->metrics.descender));
    }

    auto font_face_t::height_get() const -> std::int16_t
    {
        return ascender_get() - descender_get();
    }

    auto font_face_t::family_get() const -> std::string
    {
        if (m_handle->family_name)
        {
            return m_handle->family_name;
        }
        return std::string{};
    }

    auto font_face_t::style_get() const -> std::string
    {
        if (m_handle->style_name)
        {
            return m_handle->style_name;
        }
        return std::string{};
    }
}
