export module lgo.GUI.font.cha;

import std;

import lgo.math.vec;
import lgo.GUI.font.face;
import lgo.GUI.font.preference;
import lgo.GUI.texture;

export namespace lgo
{
    class font_char_t
    {
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
}
