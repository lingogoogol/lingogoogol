#ifndef COMPILERCPP_GUI_FREETYPE_FONT_FACE
#define COMPILERCPP_GUI_FREETYPE_FONT_FACE

#include "freetype.hpp"
#include "misc.hpp"

class font_face_t {
public:
    static auto freetype_set(freetype_t* freetype) -> void;

    font_face_t(std::string path, std::uint64_t pixel_size);
    ~font_face_t();

    auto handle_get() const -> FT_Face;
    auto leading_get() const -> std::int16_t;
    auto ascender_get() const -> std::int16_t;
    //Negative below the baseline.
    auto descender_get() const -> std::int16_t;
    //height = ascender - descender.
    auto height_get() const -> std::int16_t;
    auto family_get() const -> std::string;
    auto style_get() const -> std::string;
private:
    static freetype_t* m_freetype;

    FT_Face m_handle{};
};

freetype_t* font_face_t::m_freetype{};

auto font_face_t::freetype_set(freetype_t* freetype) -> void {
    m_freetype = freetype;
    return;
}

font_face_t::font_face_t(std::string path, std::uint64_t pixel_size) {
    FT_New_Face(m_freetype->handle_get(), path.data(), 0, &m_handle);
    FT_Set_Pixel_Sizes(m_handle, static_cast<FT_UInt>(pixel_size), static_cast<FT_UInt>(pixel_size));
    return;
}

font_face_t::~font_face_t() {
    FT_Done_Face(m_handle);
    return;
}

auto font_face_t::handle_get() const -> FT_Face {
    return m_handle;
}

auto font_face_t::leading_get() const -> std::int16_t {
    return m_handle->height;
}

auto font_face_t::ascender_get() const -> std::int16_t {
    return m_handle->ascender;
}

auto font_face_t::descender_get() const -> std::int16_t {
    return m_handle->descender;
}

auto font_face_t::height_get() const -> std::int16_t {
    return m_handle->ascender - m_handle->descender;
}

auto font_face_t::family_get() const -> std::string {
    if (m_handle->family_name) {
        return m_handle->family_name;
    }
    return std::string{};
}

auto font_face_t::style_get() const -> std::string {
    if (m_handle->style_name) {
        return m_handle->style_name;
    }
    return std::string{};
}

#endif