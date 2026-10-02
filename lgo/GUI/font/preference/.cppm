export module lgo.GUI.font.preference;

import std;

import lgo.GUI.font.face;

export namespace lgo
{
    class font_preference_t: public std::vector<font_face_t*>
    {
    public:
        using size_t = std::uint64_t;
        
        static auto default_get() -> font_preference_t&;

        auto favorite(char32_t code, std::uint32_t* index_ptr = nullptr) const -> font_face_t*;
    private:
        static font_preference_t m_default;
    };
}
