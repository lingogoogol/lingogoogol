#ifndef COMPILERCPP_CODE_SOURCE
#define COMPILERCPP_CODE_SOURCE

#include <string>
#include <fstream>

#include "../lib/file.hpp"
#include "../global/.hpp"

class source_t {
public:
    std::wstring m_path{};
    std::fstream m_file{};
    bool m_valid{ false };
    std::vector<global> m_content{};
    
    source_t() = default;
    source_t(std::wstring path);

    auto open() -> void;
    auto close() -> void;
    auto clear() -> void;
    auto save() -> void;
    auto display(divy_soft_t* editor, focus_t* focus) -> void;
};

source_t::source_t(std::wstring path): m_path{ path }, m_file{} {}

auto source_t::open() -> void {
    m_file = create_fstream(m_path);
    if (!m_file.fail()) {
        m_valid = true;
    }
    return;
}

auto source_t::close() -> void {
    m_valid = false;
    m_file.close();
    return;
}

auto source_t::clear() -> void {
    bool opened{ m_valid };
    if (opened) {
        close();
    }
    std::ofstream ofstream{ create_ofstream(m_path) };
    ofstream.close();
    if (opened) {
        open();
    }
    return;
}

auto source_t::save() -> void {
    clear();
    bool opened{ m_valid };
    if (!opened) {
        open();
    }
    for (std::size_t i{ 0 }; i < m_content.size(); ++i) {
        m_content[i].save(m_file);
    }
    if (!opened) {
        close();
    }
    return;
}

auto source_t::display(divy_soft_t* editor, focus_t* focus) -> void {
    for (std::size_t i{ 0 }; i < m_content.size(); ++i) {
        editor->add_object<fun_GUI_t>(focus, editor->space_x(std_margin), depth_range_t{ 0.0f, 1.0f }, &m_content[i].to_fun());
    }
    return;
}
    

#endif