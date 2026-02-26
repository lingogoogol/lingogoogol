#ifndef COMPILERCPP_GUI_FREETYPE_FONT_PREFERENCE
#define COMPILERCPP_GUI_FREETYPE_FONT_PREFERENCE

#include <vector>

#include "font_face.hpp"

class font_preference_t: public std::vector<font_face_t*> {
public:
    using size_t = std::uint64_t;
    
    static auto default_get() -> font_preference_t&;

    auto favorite(char32_t code, std::uint32_t* index_ptr = nullptr) const -> font_face_t*;
private:
    static font_preference_t m_default;
};

font_preference_t font_preference_t::m_default{};

auto font_preference_t::default_get() -> font_preference_t& {
    return m_default;
}

auto font_preference_t::favorite(char32_t code, std::uint32_t* index_ptr) const -> font_face_t* {
    for (size_t i{ 0 }; i < size(); ++i) {
        font_face_t* current{ (*this)[i] };
        FT_UInt index{ FT_Get_Char_Index(current->handle_get(), code) };
        if (index) {
            if (index_ptr) {
                *index_ptr = index;
            }
            return current;
        }
    }
    if (index_ptr) {
        *index_ptr = 0;
    }
    return (*this)[0];
}

#endif