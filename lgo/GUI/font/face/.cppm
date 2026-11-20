export module lgo.GUI.font.face;

import std;

import external.FreeType;

import lgo.GUI.font.FreeType;

export namespace lgo
{
    class font_face_t
    {
    public:
        static auto freetype_set(FreeType_t* freetype) -> void;
        // Framebuffer pixels per logical pixel for faces created afterwards (1 on
        // desktop). Glyphs are rasterized at the framebuffer resolution while
        // metrics stay in logical pixels, so text stays sharp on dense screens.
        static auto raster_scale_set(float scale) -> void;

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
        auto raster_scale_get() const -> float;
        // A FreeType 26.6 length in framebuffer pixels, as whole logical pixels
        // (rounded down, like `>> 6` at scale 1).
        auto logical(std::int64_t length) const -> std::int64_t;
    private:
        static FreeType_t* m_freetype;
        static float m_raster_scale_default;

        FT_Face m_handle{};
        float m_raster_scale{ 1 };
    };
}
