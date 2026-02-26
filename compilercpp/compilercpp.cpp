#include <iostream>
#include <fstream>
#include <cstdint>
#include <string>
#include <vector>
#include <filesystem>
#include <functional>

#define UNICODE

#include "lib/header.hpp"

#include "lib/.hpp"
#include "out/.hpp"
#include "global/.hpp"
#include "stmt/.hpp"
#include "expr/.hpp"
#include "instr/.hpp"

#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "D2d1.lib")
#pragma comment(lib, "Dwrite.lib")

class setting_t {
public:
    std::wstring source_filename{};
    std::wstring exe_filename{};
};

auto message_loop(engine_t* engine) -> bool {
    MSG message{};
    BOOL message_get_result{ GetMessageW(&message, NULL, 0, 0) };
    if (!message_get_result) {
        engine->set_exit();
        return true;
    }
    if (message_get_result == -1) {
        log_file("message_get\n");
        engine->set_exit();
        return true;
    }
    TranslateMessage(&message);
    DispatchMessageW(&message);
    return false;
}

auto get_input_callback(state_t* state, std::wstring* out, std::uint64_t input_id, std::uint64_t previous_state) -> void {
    *out = state->get_object<text_input_t>(input_id).lock()->get_text();
    state->restore_state(previous_state);
    state->remove_state(previous_state);
    return;
}

auto get_input(state_t* state, std::wstring prompt, std::wstring* out) -> void {
    std::uint64_t previous_state{ state->save_state() };
    state->clear_state();
    state->add_object<text_t>(prompt, pos_2D{ 0x80, 0x80 }, size_2D{ 0x300, 0x40 }
    , pos_2D{ 0x80, 0x80 }, size_2D{ 0x300, 0x40 }, size_1D{ 0x10 }, color_t{ 1.0f, 1.0f, 1.0f }, alignment_2D{ alignment_x::left, alignment_y::center });
    std::uint64_t input_id{ state->add_object<text_input_t>(pos_2D{ 0x80, 0xC0 }, size_2D{ 0x300, 0x270 }, 0.0f
    , size_1D{ 0x20 }, color_t{ 1.0f, 1.0f, 1.0f }, alignment_2D{ alignment_x::left, alignment_y::top }) };
    state->add_object<button_t>(pos_2D{ 0x1C0, 0x340 }, size_2D{ 0x80, 0x40 }, size_1D{ 0x4 }, L"確定", depth_range_t{ 0.0f, 1.0f }
    , std::bind(get_input_callback, state, out, input_id, previous_state));
    return;
}

auto compile(state_t* state, setting_t* setting) -> void {
    std::ifstream source{ create_ifstream(setting->source_filename) };
    if (source.fail()) {
        get_input(state, L"輸入原始碼檔案位置：", &setting->source_filename);
        return;
    }
    std::ofstream exe{ create_ofstream(setting->exe_filename) };
    if (exe.fail()) {
        get_input(state, L"輸入執行檔輸出位置：", &setting->exe_filename);
        return;
    }

    std::string entrance{ get_string(source) };

    std::vector<fun> funs{};
    try {
        while (true) {
            global get{ source };
            switch (get.type()) {
            case global::fun:
                funs.push_back(get.to_fun());
                break;
            default:
                log_file("global type\n");
            }
        }
    }
    catch (error& e) {
        if (e.type() != error::eof) {
            throw e;
        }
    }

    std::uint32_t bss_size{ 0 };
    std::uint32_t entry_point_address{ 0 };
    std::vector<unsigned char> data{}, rdata{}, tls{}, text{ 0x33, 0b11000000, 0xC3 }, idata{}, reloc{};
    auto entrance_fun{ std::find_if(funs.begin(), funs.end(), [&entrance] (const fun& in) {
        return in.name() == entrance;
    }) };
    entrance_fun->compile(text);
    funs.erase(entrance_fun);
    for (std::int8_t i{ 0 }; i < funs.size(); ++i) {
        funs[i].compile(text);
    }

    dos_header(exe);
    signature(exe);
    coff_file_header_missing_field coff_file_header_missing_field_obj{ coff_file_header(exe) };
    const std::streamoff start_of_optional_header{ exe.tellp() };
    optional_header_missing_field optional_header_missing_field_obj{ optional_header(exe) };
    fill_integer(exe, coff_file_header_missing_field_obj.m_size_of_optional_header
    , static_cast<std::uint16_t>(static_cast<std::streamoff>(exe.tellp()) - start_of_optional_header));

    std::vector<section> sections{};
    size_entry size_obj{};
    push_section(sections, ".text", section_flag::CNT_CODE | section_flag::MEM_EXECUTE | section_flag::MEM_READ
    , text, static_cast<std::uint32_t>(text.size()), size_obj);
    push_section(sections, ".bss", section_flag::CNT_UNINITIALIZED_DATA | section_flag::MEM_READ | section_flag::MEM_WRITE
    , {}, bss_size, size_obj);
    push_section(sections, ".data", section_flag::CNT_INITIALIZED_DATA | section_flag::MEM_READ | section_flag::MEM_WRITE
    , data, static_cast<std::uint32_t>(data.size()), size_obj);
    push_section(sections, ".rdata", section_flag::CNT_INITIALIZED_DATA | section_flag::MEM_READ
    , rdata, static_cast<std::uint32_t>(rdata.size()), size_obj);
    push_section(sections, ".tls", section_flag::CNT_INITIALIZED_DATA | section_flag::MEM_READ | section_flag::MEM_WRITE
    , tls, static_cast<std::uint32_t>(tls.size()), size_obj);
    push_section(sections, ".idata", section_flag::CNT_INITIALIZED_DATA | section_flag::MEM_READ | section_flag::MEM_WRITE
    , idata, static_cast<std::uint32_t>(idata.size()), size_obj);
    push_section(sections, ".reloc", section_flag::CNT_INITIALIZED_DATA | section_flag::MEM_READ | section_flag::MEM_DISCARDABLE
    , reloc, static_cast<std::uint32_t>(reloc.size()), size_obj);

    std::vector<section_header_missing_field> section_header_missing_fields{};
    for (std::uint8_t i{ 0 }; i < sections.size(); ++i) {
        section_header_missing_fields.push_back(pad_section_header(exe, sections[i]));
    }
    std::uint32_t virtual_address{ static_cast<std::uint32_t>(exe.tellp()) };
    align_file(exe, virtual_address);
    fill_integer(exe, coff_file_header_missing_field_obj.m_number_of_sections, static_cast<std::uint16_t>(sections.size()));
    fill_integer(exe, optional_header_missing_field_obj.m_size_of_code, size_obj.m_code);
    fill_integer(exe, optional_header_missing_field_obj.m_size_of_initialized_data, size_obj.m_initialized_data);
    fill_integer(exe, optional_header_missing_field_obj.m_size_of_uninitialized_data, size_obj.m_uninitialized_data);
    fill_integer(exe, optional_header_missing_field_obj.m_size_of_headers, static_cast<std::uint32_t>(exe.tellp()));
    fill_integer(exe, optional_header_missing_field_obj.m_address_of_entry_point, virtual_address + entry_point_address);
    fill_integer(exe, optional_header_missing_field_obj.m_base_of_code, virtual_address);

    for (std::uint8_t i{ 0 }; i < sections.size(); ++i) {
        pad_section(exe, sections[i], section_header_missing_fields[i], virtual_address);
    }
    fill_integer(exe, optional_header_missing_field_obj.m_size_of_image, virtual_address);

    source.close();
    exe.close();
    return;
}

auto change_setting() -> void {
    return;
}

auto edit(state_t* state, setting_t* setting, std::weak_ptr<text_input_t> source_filename_input) -> void {
    state->save_state();
    state->clear_state();
    std::wstring source_filename{ source_filename_input.lock()->get_text() };
    setting->source_filename = source_filename;
    state->add_object<text_t>(source_filename, pos_2D{ 0x0, 0x0 }, size_2D{ 0x300, 0x40 }
    , pos_2D{ 0x0, 0x0 }, size_2D{ 0x300, 0x40 }, size_1D{ 0x10 }, color_t{ 1.0f, 1.0f, 1.0f }, alignment_2D{ alignment_x::left, alignment_y::center });
    state->add_object<button_t>(pos_2D{ 0x300, 0x0 }, size_2D{ 0x80, 0x40 }, size_1D{ 0x4 }
    , L"更改設定", depth_range_t{ 0.0f, 1.0f }, std::bind(change_setting));
    state->add_object<button_t>(pos_2D{ 0x380, 0x0 }, size_2D{ 0x80, 0x40 }, size_1D{ 0x4 }
    , L"開始編譯", depth_range_t{ 0.0f, 1.0f }, std::bind(compile, state, setting));
    return;
}

auto WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR arg, int) -> int {
    init_logfile(arg);
    init_directx();
    engine_t engine{ instance, size_2D{ 0x400, 0x400 } };
    state_t state{ &engine };
    setting_t setting{};
    std::uint64_t source_filename_input_id{ state.add_object<text_input_t>(pos_2D{ 0x100, 0x1E0 }, size_2D{ 0x170, 0x40 }, 0.0f
    , size_1D{ 0x20 }, color_t{ 1.0f, 1.0f, 1.0f }, alignment_2D{ alignment_x::left, alignment_y::top }) };
    state.add_object<button_t>(pos_2D{ 0x280, 0x1E0 }, size_2D{ 0x80, 0x40 }, size_1D{ 0x4 }
    , L"開啟原始碼", depth_range_t{ 0.0f, 1.0f }, std::bind(edit, &state, &setting, state.get_object<text_input_t>(source_filename_input_id)));
    while (!message_loop(&engine)) {}

    log_file("complete\n");
    return 0;
}