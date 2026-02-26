#ifndef COMPILERCPP_LIB_GUI_TEXT_INPUT
#define COMPILERCPP_LIB_GUI_TEXT_INPUT

#include <functional>

#include "object.hpp"
#include "text.hpp"

#include "../stu.hpp"
#include "../GUI_primitive/engine_decl.hpp"

class text_input_t: public GUI_object {
private:
    engine_t* m_engine{};
    text_t m_text{};
    std::function<void(wchar_t)>* m_callback{};

    auto callback(wchar_t in) -> void;
public:
    text_input_t() = default;
    text_input_t(engine_t* engine, pos_2D pos, size_2D size, size_1D size_font, color_t color, alignment_2D alignment);

    auto show() -> void override;
    auto hide() -> void override;
};

auto text_input_t::callback(wchar_t in) -> void {
    std::wstring text{ m_text.get_text() };
    text.push_back(in);
    m_text.set_text(text);
    return;
}

text_input_t::text_input_t(engine_t* engine, pos_2D pos, size_2D size, size_1D size_font, color_t color, alignment_2D alignment)
: m_engine{ engine }, m_text{ engine, L"", pos, size, pos, size, size_font, color, alignment } {}

auto text_input_t::show() -> void {
    m_text.show();
    m_callback = m_engine->add_charw(std::bind(&text_input_t::callback, this, std::placeholders::_1));
    return;
}

auto text_input_t::hide() -> void {
    m_text.hide();
    m_engine->remove_charw(m_callback);
    return;
}

#endif