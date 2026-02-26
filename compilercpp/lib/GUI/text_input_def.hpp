#ifndef COMPILERCPP_LIB_GUI_TEXT_INPUT_DEF
#define COMPILERCPP_LIB_GUI_TEXT_INPUT_DEF

#include <functional>

#include "object.hpp"
#include "text_def.hpp"
#include "engine_decl.hpp"
#include "stu.hpp"

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

#endif