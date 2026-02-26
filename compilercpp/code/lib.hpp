#ifndef COMPILERCPP_CODE_LIB
#define COMPILERCPP_CODE_LIB

#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <filesystem>

#include "../lib/.hpp"
#include "../out/constant.hpp"

class lib_t {
private:
    std::filesystem::path m_path{};
    std::ifstream m_file{};
    std::map<std::string, std::ifstream::pos_type> m_data{};
public:
    lib_t(std::filesystem::path path);

    auto contain_symbol(std::string name) -> bool;
    auto get_symbol(std::string name) -> std::string;
    auto get_name() -> std::string;
};

lib_t::lib_t(std::filesystem::path path): m_path{ path }, m_file{ create_ifstream(path) } {
    if (get_string(m_file, 8) != "!<arch>\n") {
        throw error_t{ "這個檔案" + path.wstring() + L"不是程式庫檔案" };
    }
    m_file.seekg(m_file.tellg() + std::streamoff{ 48 });
    auto linker_member1_len{ std::stoi(get_string(m_file, 10)) };
    m_file.seekg(m_file.tellg() + std::streamoff{ 2 + linker_member1_len + 60 });
    auto member_count{ get_integer<unsigned long>(m_file) };
    std::vector<unsigned long> member_pos{};
    for (unsigned long i{ 0 }; i < member_count; ++i) {
        member_pos.push_back(get_integer<unsigned long>(m_file));
    }
    auto symbol_count{ get_integer<unsigned long>(m_file) };
    std::vector<unsigned short> symbol_member{};
    for (unsigned long i{ 0 }; i < symbol_count; ++i) {
        symbol_member.push_back(get_integer<unsigned short>(m_file));
    }
    for (unsigned long i{ 0 }; i < symbol_count; ++i) {
        m_data.emplace(get_string_null_terminated(m_file), member_pos[symbol_member[i] - 1]);
    }
    return;
}

auto lib_t::contain_symbol(std::string name) -> bool {
    return m_data.count(name);
}

auto lib_t::get_symbol(std::string name) -> std::string {
    m_file.seekg(m_data[name] + std::streamoff{ 60 });
    m_file.seekg(m_file.tellg() + std::streamoff{ 20 });
    get_string_null_terminated(m_file);
    std::string dll_path{ get_string_null_terminated(m_file) };
    return dll_path;
}

auto lib_t::get_name() -> std::string {
    return m_path.filename().string();
}

auto search_symbol(std::vector<lib_t>& lib, std::string name) -> std::string {
    for (std::size_t i{ 0 }; i < lib.size(); ++i) {
        if (lib[i].contain_symbol(name)) {
            return lib[i].get_symbol(name);
        }
    }
    throw error_t{ "沒有找到符號\"" + to_wstring(name) + L"\"" };
}

#endif