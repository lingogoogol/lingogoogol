#ifndef COMPILERCPP_LIB_GUI_TEXT_INPUT
#define COMPILERCPP_LIB_GUI_TEXT_INPUT

#include <functional>

#include "text_scroll.hpp"

#include "../other/stu.hpp"
#include "../GUI_primitive/engine_decl.hpp"

class text_input_t: public text_scroll_t {
private:
    engine_t* m_engine{};
    std::function<void(wchar_t)>* m_callback{};

    auto text_input_callback(wchar_t in) -> void;
public:
    text_input_t() = default;
    text_input_t(engine_t* engine, pos_2D pos, size_2D size, float depth, size_1D size_font, color_t color, alignment_2D alignment);

    auto show() -> void override;
    auto hide() -> void override;
};

auto text_input_t::text_input_callback(wchar_t in) -> void {
    std::wstring text{ get_text() };
    if (in == '\b') {
        if (!text.empty()) {
            text.pop_back();
        }
    }
    else {
        text.push_back(in);
    }
    set_text(text);
    return;
}

text_input_t::text_input_t(engine_t* engine, pos_2D pos, size_2D size, float depth, size_1D size_font, color_t color, alignment_2D alignment)
: text_scroll_t{ engine, L"", pos, size, depth, size_font, color, alignment }, m_engine{ engine } {}

auto text_input_t::show() -> void {
    text_scroll_t::show();
    m_callback = m_engine->add_charw(std::bind(&text_input_t::text_input_callback, this, std::placeholders::_1));
    return;
}

auto text_input_t::hide() -> void {
    text_scroll_t::hide();
    m_engine->remove_charw(m_callback);
    return;
}

#endif