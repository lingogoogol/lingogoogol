#ifndef COMPILERCPP_LIB_GUI_TEXT_IMPL
#define COMPILERCPP_LIB_GUI_TEXT_IMPL

text_t::text_t(engine_t* engine, std::wstring text, pos_2D pos, size_2D size, size_1D size_font, color_t color, alignment_2D alignment)
: m_engine{ engine }, m_text{ text }, m_pos{ pos }, m_size{ size }, m_size_font{ size_font }, m_color{ color }, m_alignment{ alignment } {}

auto text_t::show() -> void {
    m_primitive = m_engine->add_text(m_text, m_pos, m_size, m_size_font, m_color, m_alignment);
    return;
}

auto text_t::hide() -> void {
    m_engine->remove_text(m_primitive);
    return;
}

auto text_t::get_text() -> std::wstring {
    return m_text;
}

auto text_t::set_text(std::wstring text) -> void {
    m_text = text;
    m_engine->remove_text(m_primitive);
    m_primitive = m_engine->add_text(m_text, m_pos, m_size, m_size_font, m_color, m_alignment);
    return;
}

#endif