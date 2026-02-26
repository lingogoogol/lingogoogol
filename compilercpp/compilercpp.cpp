#include <iostream>
#include <fstream>
#include <cstdint>
#include <string>
#include <vector>
#include <filesystem>
#include <functional>
#include <tuple>

#define UNICODE

#include "lib/.hpp"

const ver_t version_current{ 0x0, 0x0, 0x4, 0x1 };
const ver_t version_minimum{ 0x0, 0x0, 0x4, 0x0 };

#include "GUI/.hpp"
#include "code/.hpp"
#include "global/.hpp"
#include "stmt/.hpp"
#include "expr/.hpp"
#include "instr/.hpp"
#include "setting/.hpp"
#include "out/.hpp"

template<typename t_member>
auto save_setting(state_t* state, setting_t* setting, std::uint64_t input_id) -> void {
    setting->set_member<t_member>(state->get_object_shared<std_text_block_scroll_input_t>(input_id)->get_text());
    return;
}

auto edit(engine_t* engine, state_t* state, setting_t* setting) -> void;

auto change_setting_callback(engine_t* engine, state_t* state, setting_t* setting, focus_t* focus) -> void {
    focus->clear();
    delete focus;
    edit(engine, state, setting);
    return;
}

template<typename t_member>
auto change_setting_add_string_member(setting_t* setting, focus_t* focus, std::shared_ptr<divy_hard_t> div, std::wstring prompt) -> void {
    auto source_path_left{ div->add_object_ghost_shared<divx_soft_t>(div_create_from_member{}, std_forward, std_margin
    , div->space_x(std_margin), alignment_2D{ alignment_x::left, alignment_y::center }) };
    auto source_path_right{ div->add_object_shared<divx_soft_t>(div_create_from_member{}, std_forward, std_margin
    , div->space_x(std_margin), alignment_2D{ alignment_x::right, alignment_y::center }) };
    source_path_left->add_object<std_text_line_t>(prompt, size_1D{ 0x100 }, std_white);
    auto source_path_input_id{ div->add_object<std_text_block_scroll_input_t>(0.0f, setting->get_member<t_member>(), focus, div->space_x(std_margin)) };
    source_path_right->add_object<button_classic_std_t>(std::bind(save_setting<t_member>, div.get(), setting, source_path_input_id), L"儲存", depth_range_t{ 0.0f, 1.0f });
    return;
}

auto change_setting(engine_t* engine, state_t* state, setting_t* setting) -> void {
    state->clear_state();
    focus_t* focus{ new focus_t{} };

    auto main_div_left{ state->add_object_shared<divy_hard_t>(pos_2D{ 0, 0 }
    , size_2D{ engine->get_window_size().x / 2, engine->get_window_size().y }, size_1D{ 0 }, alignment_2D{ alignment_x::left, alignment_y::top }) };
    auto main_div_right{ state->add_object_shared<divy_hard_t>(pos_2D{ engine->get_window_size().x / 2, 0 }
    , size_2D{ engine->get_window_size().x / 2, engine->get_window_size().y }, size_1D{ 0 }, alignment_2D{ alignment_x::left, alignment_y::top }) };

    change_setting_add_string_member<setting_source_path_t>(setting, focus, main_div_left, L"原始碼位置：");
    change_setting_add_string_member<setting_lib_path_t>(setting, focus, main_div_left, L"程式庫位置：");
    change_setting_add_string_member<setting_exe_path_t>(setting, focus, main_div_left, L"執行檔輸出位置：");
    change_setting_add_string_member<setting_entrance_fun_t>(setting, focus, main_div_left, L"入口點函數名稱：");

    auto main_div_bottom{ state->add_object_shared<divx_hard_t>(pos_2D{ 0, 0 }, engine->get_window_size()
    , size_1D{ 0 }, alignment_2D{ alignment_x::right, alignment_y::bottom }) };
    main_div_bottom->add_object<button_classic_std_t>(std::bind(change_setting_callback, engine, state, setting, focus), L"確定", depth_range_t{ 0.0f, 1.0f });
    return;
}

auto compile(state_t* state, setting_t* setting, code_t* code) -> void {
    std::vector<error_t> error_queue{};
    std::wstring message{};
    try {
        code->compile(setting, &error_queue);
    }
    catch (error_t error) {
        message += L"fatal error: " + to_wstring(error.what()) + L'\n';
    }
    if (error_queue.size()) {
        for (std::size_t i{ 0 }; i < error_queue.size(); ++i) {
            message += to_wstring(error_queue[i].what()) + L'\n';
        }
    }
    else if (!message.size()) {
        message = L"編譯成功！";
    }
    inform_restore(state, message);
    return;
}

auto edit(engine_t* engine, state_t* state, setting_t* setting) -> void {
    static code_t code{};
    static std::wstring source_path{};
    if (source_path != setting->get_member<setting_source_path_t>()) {
        code.clear();
        std::vector<error_t> error_queue{};
        try {
            code.add_source(setting->get_member<setting_source_path_t>(), &error_queue);
        }
        catch (error_t error) {}
    }
    source_t* main_source{ &code.m_source[setting->get_member<setting_source_path_t>()] };
    main_source->open();
    state->clear_state();
    auto main_div{ state->add_object_shared<divy_hard_t>(pos_2D{ 0, 0 }, engine->get_window_size()
    , size_1D{ 0 }, alignment_2D{ alignment_x::left, alignment_y::top }) };

    auto menu_left{ main_div->add_object_ghost_shared<divx_soft_t>(div_create_from_member{}, std_forward, std_margin
    , main_div->space_x(std_margin), alignment_2D{ alignment_x::left, alignment_y::center }) };
    std::wstring title{};
    if (main_source->m_valid) {
        title = setting->get_member<setting_source_path_t>();
    }
    else {
        title = L"找不到原始碼檔案";
    }
    menu_left->add_object<std_text_line_t>(title, main_div->space_x(std_margin), std_white);

    auto menu_right{ main_div->add_object_shared<divx_soft_t>(div_create_from_member{}, std_forward, std_margin
    , main_div->space_x(std_margin), alignment_2D{ alignment_x::right, alignment_y::center }) };
    menu_right->add_object<button_classic_std_t>(std::bind(change_setting, engine, state, setting), L"更改設定", depth_range_t{ 0.0f, 1.0f });
    menu_right->add_object<button_classic_std_t>(std::bind(&source_t::save, main_source), L"儲存檔案", depth_range_t{ 0.0f, 1.0f });
    menu_right->add_object<button_classic_std_t>(std::bind(compile, state, setting, &code), L"開始編譯", depth_range_t{ 0.0f, 1.0f });

    main_div->add_object_shared<std_linex_t>(main_div->space_x(std_margin), std_white, 0.0f);
    auto center_div{ main_div->add_object_shared<divx_soft_t>(main_div->space_2D(std_margin), std_margin, alignment_2D{ alignment_x::left, alignment_y::top }) };
    auto tree_div{ center_div->add_object_shared<divy_soft_t>(size_2D{ 0x100, center_div->space_y(std_margin).x }
    , std_margin, alignment_2D{ alignment_x::left, alignment_y::top }) };
    center_div->add_object<std_liney_t>(center_div->space_y(std_margin), std_white, 0.0f);
    auto editor_div{ center_div->add_object_shared<divy_soft_t>(center_div->space_2D(std_margin), std_margin, alignment_2D{ alignment_x::left, alignment_y::top }) };
    main_source->display(editor_div.get(), state->get_focus());
    
    main_source->close();
    return;
}

auto open_logfile(engine_t* engine, state_t* state, setting_t* setting) -> void;

auto open_logfile_callback(engine_t* engine, state_t* state, setting_t* setting, std::wstring* logfile_path) -> void {
    setting->set_member<setting_logfile_path_t>(*logfile_path);
    delete logfile_path;
    open_logfile(engine, state, setting);
    return;
}

auto open_logfile(engine_t* engine, state_t* state, setting_t* setting) -> void {
    if (!init_logfile(to_string(setting->get_member<setting_logfile_path_t>()))) {
        std::wstring* logfile_path{ new std::wstring{} };
        get_input(state, L"記錄檔位置：", logfile_path, std::bind(open_logfile_callback, engine, state, setting, logfile_path));
    }
    else {
        edit(engine, state, setting);
    }
    return;
}

auto WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) -> int {
    log_file("CWD: " + std::filesystem::current_path().string() + "\n");
    init_directx();
    engine_t engine{ instance, size_2D{ 0x600, 0x400 } };
    depth_tracker_t depth_tracker{};
    state_t state{ &engine, &depth_tracker };
    int argc{};
    LPWSTR* argv{ CommandLineToArgvW(GetCommandLineW(), &argc) };
    std::wstring setting_path{};
    if (argc >= 2) {
        setting_path = argv[1];
    }
    if (argc > 2) {
        log_file("參數數量不正確，不能超過一個\n");
    }
    setting_t setting{};
    setting.parse(setting_path);
    open_logfile(&engine, &state, &setting);
    while (!engine.message_loop()) {}

    setting.save();
    state.clear_state();
    log_file("完成\n");
    return 0;
}