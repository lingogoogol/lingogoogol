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
        static FreeType_t* m_freetype;

        FT_Face m_handle{};
    };
}
