#ifndef COMPILERCPP_SETTING_MAIN_IMPL
#define COMPILERCPP_SETTING_MAIN_IMPL

#include "main_def.hpp"
#include "logfile_path.hpp"
#include "source_path.hpp"
#include "exe_path.hpp"
#include "entrance_fun.hpp"
#include "lib_path.hpp"

#include "../lib/.hpp"

setting_t::~setting_t() {
    for (auto i{ m_data.begin() }; i != m_data.end(); ++i) {
        delete i->second;
    }
    return;
}

auto setting_t::parse(std::wstring path) -> void {
    if (path.empty()) {
        m_path = default_path;
    }
    else {
        m_path = path;
    }
    m_file = create_fstream(m_path);
    m_file.seekg(0, std::ios_base::end);
    std::streampos file_end{ m_file.tellg() };
    if (file_end == 0) {
        return;
    }
    m_file.seekg(0);
    ver_t ver{ m_file };
    if (ver < version_minimum) {
        throw error_t{ "設定檔案版本過舊，請先更新" };
    }
    while (m_file.tellg() != file_end) {
        switch (get_integer<type::type_t>(m_file)) {
        case type::logfile_path:
            m_data[type::logfile_path] = setting_logfile_path_t::get(m_file);
            break;
        case type::source_path:
            m_data[type::source_path] = setting_source_path_t::get(m_file);
            break;
        case type::lib_path:
            m_data[type::lib_path] = setting_lib_path_t::get(m_file);
            break;
        case type::exe_path:
            m_data[type::exe_path] = setting_exe_path_t::get(m_file);
            break;
        case type::entrance_fun:
            m_data[type::entrance_fun] = setting_entrance_fun_t::get(m_file);
            break;
        default:
            throw error_t{ "設定種類錯誤" };
        }
    }
    return;
}

template<typename t_member>
auto setting_t::get_member() -> decltype(t_member::get_data_default()) {
    if (m_data.contains(t_member::type)) {
        return *dynamic_cast<t_member*>(m_data[t_member::type])->get_data_ptr();
    }
    else {
        return t_member::get_data_default();
    }
}

template<typename t_member, typename t_in>
auto setting_t::set_member(t_in&& in) -> void {
    t_member* member{};
    if (m_data.contains(t_member::type)) {
        member = dynamic_cast<t_member*>(m_data[t_member::type]);
    }
    else {
        member = new t_member{};
        m_data[t_member::type] = member;
    }
    member->set_data(std::forward<t_in>(in));
    return;
}

auto setting_t::save() -> void {
    m_file.close();
    m_file.open(m_path, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
    version_current.pad(m_file);
    for (auto i{ m_data.begin() }; i != m_data.end(); ++i) {
        pad_integer(m_file, i->first);
        i->second->pad(m_file);
    }
    return;
}

#endif