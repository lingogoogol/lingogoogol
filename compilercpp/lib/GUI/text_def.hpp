#ifndef COMPILERCPP_LIB_GUI_TEXT_DEF
#define COMPILERCPP_LIB_GUI_TEXT_DEF

#include <string>

#include "object.hpp"
#include "stu.hpp"
#include "engine_def.hpp"

class text_t: public GUI_object {
private:
    engine_t* m_engine{};
    std::wstring m_text{};
    pos_2D m_pos{};
    size_2D m_size{};
    size_1D m_size_font{};
    color_t m_color{};
    alignment_2D m_alignment{};
    text_primitive_t* m_primitive{};
public:
    text_t() = default;
    text_t(engine_t* engine, std::wstring m_text, pos_2D pos, size_2D size, size_1D size_font, color_t color, alignment_2D alignment);

    auto show() -> void override;
    auto hide() -> void override;

    auto get_text() -> std::wstring;
    auto set_text(std::wstring text) -> void;
};

#endif