module lgo.GUI.font.preference;

import std;

import external.FreeType;

import lgo.GUI.font.face;

namespace lgo
{
    font_preference_t font_preference_t::m_default{};

    auto font_preference_t::default_get() -> font_preference_t&
    {
        return m_default;
    }

    auto font_preference_t::favorite(char32_t code, std::uint32_t* index_ptr) const -> font_face_t*
    {
        for (size_t i{ 0 }; i < size(); ++i)
        {
            font_face_t* current{ (*this)[i] };
            FT_UInt index{ FT_Get_Char_Index(current->handle_get(), code) };
            if (index)
            {
                if (index_ptr)
                {
                    *index_ptr = index;
                }
                return current;
            }
        }
        if (index_ptr)
        {
            *index_ptr = 0;
        }
        return (*this)[0];
    }
}
