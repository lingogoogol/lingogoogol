#ifndef COMPILERCPP_CODE_MAIN
#define COMPILERCPP_CODE_MAIN

#include <map>
#include <string>
#include <filesystem>
#include <vector>

#include "source.hpp"
#include "lib.hpp"

#include "../setting/.hpp"
#include "../lib/.hpp"
#include "../GUI/.hpp"
#include "../global/.hpp"
#include "../out/.hpp"

class code_t {
public:
    std::map<std::wstring, source_t> m_source{};
    std::vector<fun*> m_fun{};

    code_t() = default;
    
    auto clear() -> void;
    auto add_source(std::wstring path, std::vector<error_t>* error_queue) -> void;
    auto compile(setting_t* setting, std::vector<error_t>* error_queue) -> void;
};

auto code_t::clear() -> void {
    m_source.clear();
    m_fun.clear();
    return;
}

auto code_t::add_source(std::wstring path, std::vector<error_t>* error_queue) -> void {
    m_source[path] = source_t{ path };
    m_source[path].open();
    if (!m_source[path].m_valid) {
        return;
    }
    std::fstream& file{ m_source[path].m_file };
    try {
        while (true) {
            global current{ file };
            switch (current.type()) {
            case global::fun:
                m_fun.push_back(&current.to_fun());
                break;
            default:
                throw error_t{ "全域物件類型不正確" };
            }
            m_source[path].m_content.push_back(current);
        }
    }
    catch (error_t& error) {
        if (error.message() != L"全域物件結束") {
            throw error;
        }
    }
    m_source[path].close();
    return;
}

auto code_t::compile(setting_t* setting, std::vector<error_t>* error_queue) -> void {
    std::vector<lib_t> lib{};
    std::wstring lib_path_str{ setting->get_member<setting_lib_path_t>() };
    std::filesystem::path lib_path{ lib_path_str };
    if (!std::filesystem::is_directory(lib_path)) {
        throw error_t{ "設定的程式庫位置\"" + lib_path_str + L"\"不是資料夾" };
    }
    for (std::filesystem::directory_iterator i{ lib_path }; i != std::filesystem::directory_iterator{}; ++i) {
        lib.push_back(lib_t{ i->path() });
    }

    source_t* main_source{ &m_source[setting->get_member<setting_source_path_t>()] };
    main_source->open();
    if (!main_source->m_valid) {
        throw error_t{ "原始碼檔案位置不正確" };
    }

    code_stream data{}, rdata{}, tls{}, text{}, idata{}, reloc{}, bss{};
    auto entrance_fun{ std::find_if(m_fun.begin(), m_fun.end(), [this, setting] (const fun* in) {
        return in->name() == setting->get_member<setting_entrance_fun_t>();
    }) };
    if (entrance_fun == m_fun.end()) {
        throw error_t{ "找不到入口點函數" };
        return;
    }
    text.pad_address_def("entry_point");
    (*entrance_fun)->compile(text);
    for (auto i{ m_fun.begin() }; i != m_fun.end(); ++i) {
        if (i != entrance_fun) {
            (*i)->compile(text);
        }
    }
    main_source->close();

    std::map<std::string, std::set<std::string>> external{};
    for (auto i{ 0 }; i < text.get_ref().size(); ++i) {
        const std::string& symbol_name{ text.get_ref()[i].m_name };
        bool found{ false };
        if (text.get_def().contains(symbol_name)) {
            found = true;
        }
        for (auto j{ 0 }; j < lib.size(); ++j) {
            if (!lib[j].contain_symbol(symbol_name)) {
                continue;
            }
            if (found) {
                throw error_t{ "在" + to_wstring(lib[j].get_name()) + L"裡找到重複定義的符號" + to_wstring(symbol_name) + L"。" };
            }
            found = true;
            external[lib[j].get_symbol(symbol_name)].insert(symbol_name);
        }
    }
    idata.pad_address_def("import_table_begin");
    for (auto i{ external.begin() }; i != external.end(); ++i) {
        idata.pad_address_ref(i->first + "_import_lookup_table", code_stream::resolver_overwrite_address(code_stream::RVA, 4));
        idata.pad_int(std::uint32_t{});
        idata.pad_int(std::uint32_t{});
        idata.pad_int(std::uint32_t{});
        idata.pad_address_ref(i->first + "_name", code_stream::resolver_overwrite_address(code_stream::RVA, 4));
        idata.pad_int(std::uint32_t{});
        idata.pad_address_ref(i->first + "_import_address_table", code_stream::resolver_overwrite_address(code_stream::RVA, 4));
        idata.pad_int(std::uint32_t{});
    }
    idata.pad_int(std::uint32_t{});
    idata.pad_int(std::uint32_t{});
    idata.pad_int(std::uint32_t{});
    idata.pad_int(std::uint32_t{});
    idata.pad_int(std::uint32_t{});
    idata.pad_address_def("import_table_end");
    for (auto i{ external.begin() }; i != external.end(); ++i) {
        idata.pad_address_def(i->first + "_import_lookup_table");
        for (auto j{ i->second.begin() }; j != i->second.end(); ++j) {
            idata.pad_address_ref(*j + "_hint_name_table", code_stream::resolver_overwrite([] (const code_stream::get_address_t& get_address, const std::string name) -> std::uint64_t {
                return get_address(name, code_stream::RVA);
            }, 8));
            idata.pad_int(std::uint64_t{});
        }
        idata.pad_int(std::uint64_t{});
    }
    for (auto i{ external.begin() }; i != external.end(); ++i) {
        for (auto j{ i->second.begin() }; j != i->second.end(); ++j) {
            idata.pad_address_def(*j + "_hint_name_table");
            idata.pad_int(std::int16_t{});
            idata.pad_str(*j);
            idata.pad_byte(0);
            if (!(j->size() % 2)) {
                idata.pad_byte(0);
            }
        }
        idata.pad_address_def(i->first + "_name");
        idata.pad_str(i->first);
        idata.pad_byte(0);
    }
    idata.pad_address_def("IAT_begin");
    for (auto i{ external.begin() }; i != external.end(); ++i) {
        idata.pad_address_def(i->first + "_import_address_table");
        for (auto j{ i->second.begin() }; j != i->second.end(); ++j) {
            idata.pad_address_def("symbol_" + *j);
            idata.pad_address_ref(*j + "_hint_name_table", code_stream::resolver_overwrite([] (const code_stream::get_address_t& get_address, const std::string name) -> std::uint64_t {
                return get_address(name, code_stream::RVA);
            }, 8));
            idata.pad_int(std::uint64_t{});
        }
        idata.pad_int(std::uint64_t{});
    }
    idata.pad_address_def("IAT_end");

    if (error_queue->size()) {
        return;
    }

    std::vector<section_t> section{
        { ".text", &text, section_flag::CNT_CODE | section_flag::MEM_EXECUTE | section_flag::MEM_READ },
        { ".bss", &bss, section_flag::CNT_UNINITIALIZED_DATA | section_flag::MEM_READ | section_flag::MEM_WRITE },
        { ".data", &data, section_flag::CNT_INITIALIZED_DATA | section_flag::MEM_READ | section_flag::MEM_WRITE },
        { ".rdata", &rdata, section_flag::CNT_INITIALIZED_DATA | section_flag::MEM_READ },
        { ".tls", &tls, section_flag::CNT_INITIALIZED_DATA | section_flag::MEM_READ | section_flag::MEM_WRITE },
        { ".idata", &idata, section_flag::CNT_INITIALIZED_DATA | section_flag::MEM_READ | section_flag::MEM_WRITE },
        { ".reloc", &reloc, section_flag::CNT_INITIALIZED_DATA | section_flag::MEM_READ | section_flag::MEM_DISCARDABLE }
    };
    for (std::uint8_t i{ 0 }; i < section.size();) {
        if (!section[i].m_segment->plain_size()) {
            section.erase(section.begin() + i);
        }
        else {
            ++i;
        }
    }

    code_stream stream{};
    code_stream_def_index_counter_t index_counter{};
    stream.pad_address_def("image_begin");
    stream.pad_address_def("header_begin");
    dos_header(stream);
    signature(stream);
    coff_file_header(stream, static_cast<std::uint16_t>(section.size()));
    stream.pad_address_def("optional_header_begin");
    optional_header(stream);
    stream.pad_address_def("optional_header_end");

    for (std::uint8_t i{ 0 }; i < section.size(); ++i) {
        pad_section_header(stream, section[i]);
    }
    stream.align();
    stream.pad_address_def("header_end");
    
    for (std::size_t i{ 0 }; i < section.size(); ++i) {
        auto pad_def{
            [&section, i, &stream, &index_counter] (std::string suffix) {
                stream.pad_address_def(section[i].m_name + "_aligned" + suffix);
                if (section[i].m_characteristic & section_flag::CNT_CODE) {
                    stream.pad_address_def_index("code" + suffix, index_counter);
                }
                else if (section[i].m_characteristic & section_flag::CNT_INITIALIZED_DATA) {
                    stream.pad_address_def_index("initialized_data" + suffix, index_counter);
                }
                else if (section[i].m_characteristic & section_flag::CNT_UNINITIALIZED_DATA) {
                    stream.pad_address_def_index("uninitialized_data" + suffix, index_counter);
                }
                else {
                    throw internal_error_t{ "PE檔段落特徵錯誤" };
                }
            }
        };
        stream.pad_address_def(section[i].m_name + "_begin");
        pad_def("_begin");
        stream.merge(*section[i].m_segment);
        stream.pad_address_def(section[i].m_name + "_end");
        stream.align();
        pad_def("_end");
    }
    stream.pad_address_def("image_end");

    std::ofstream exe{ create_ofstream(setting->get_member<setting_exe_path_t>()) };
    if (exe.fail()) {
        throw error_t{ "執行檔輸出位置不正確" };
    }
    stream.resolve();
    stream.out(exe);
    exe.close();
    return;
}

#endif