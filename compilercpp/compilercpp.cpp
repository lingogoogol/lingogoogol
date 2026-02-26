#include <iostream>
#include <fstream>
#include <cstdint>
#include <string>
#include <vector>
#include <filesystem>
#include <functional>

#define UNICODE

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

const ver_t version_current{ 0x0, 0x0, 0x4, 0x0 };
const ver_t version_minimum{ 0x0, 0x0, 0x4, 0x0 };

class setting_member_t {
public:
    virtual ~setting_member_t() = default;
};

class setting_t {
private:
    static const std::wstring default_setting_path;

    std::wstring m_path{};
    std::fstream m_file{};
public:
    class type {
    public:
        using type_t = std::uint8_t;
        enum: type_t {
            logfile_path,
            source_path,
            exe_path
        };
    };

    std::map<type::type_t, setting_member_t*> m_data{};

    setting_t(std::wstring setting_path);
    ~setting_t();

    template<typename t_member>
    auto get_member() -> decltype(t_member::get_data_default());
    template<typename t_member, typename t_in>
    auto set_member(t_in&& in) -> void;

    auto save() -> void;
};

class setting_logfile_path_t: public setting_member_t {
public:
    constexpr static setting_t::type::type_t type{ setting_t::type::logfile_path };
    std::wstring m_data{};

    static auto get(std::istream& src) -> setting_logfile_path_t* {
        setting_logfile_path_t* out{ new setting_logfile_path_t{} };
        out->m_data = get_stringw(src);
        return out;
    }

    auto pad(std::ostream& dest) -> void {
        pad_stringw(dest, m_data);
        return;
    }

    auto get_data_ptr() -> std::wstring* {
        return &m_data;
    }

    static auto get_data_default() -> std::wstring {
        return L"";
    }

    auto set_data(std::wstring in) -> void {
        m_data = in;
        return;
    }
};

class setting_source_path_t: public setting_member_t {
public:
    constexpr static setting_t::type::type_t type{ setting_t::type::source_path };
    std::wstring m_data{};

    static auto get(std::istream& src) -> setting_source_path_t* {
        setting_source_path_t* out{ new setting_source_path_t{} };
        out->m_data = get_stringw(src);
        return out;
    }

    auto pad(std::ostream& dest) -> void {
        pad_stringw(dest, m_data);
        return;
    }

    auto get_data_ptr() -> std::wstring* {
        return &m_data;
    }

    static auto get_data_default() -> std::wstring {
        return L"";
    }

    auto set_data(std::wstring in) -> void {
        m_data = in;
        return;
    }
};

class setting_exe_path_t: public setting_member_t {
public:
    constexpr static setting_t::type::type_t type{ setting_t::type::exe_path };
    std::wstring m_data{};

    static auto get(std::istream& src) -> setting_exe_path_t* {
        setting_exe_path_t* out{ new setting_exe_path_t{} };
        out->m_data = get_stringw(src);
        return out;
    }

    auto pad(std::ostream& dest) -> void {
        pad_stringw(dest, m_data);
        return;
    }

    auto get_data_ptr() -> std::wstring* {
        return &m_data;
    }

    static auto get_data_default() -> std::wstring {
        return L"";
    }

    auto set_data(std::wstring in) -> void {
        m_data = in;
        return;
    }
};

const std::wstring setting_t::default_setting_path{ L"setting" };

const std::wstring default_logfile_path{ L"logfile.txt" };

setting_t::setting_t(std::wstring setting_path) {
    if (setting_path.empty()) {
        m_path = default_setting_path;
    }
    else {
        m_path = setting_path;
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
        log_file("setting_version\n");
    }
    while (m_file.tellg() != file_end) {
        switch (get_integer<type::type_t>(m_file)) {
        case type::logfile_path:
            m_data[type::logfile_path] = setting_logfile_path_t::get(m_file);
            break;
        case type::source_path:
            m_data[type::source_path] = setting_source_path_t::get(m_file);
            break;
        case type::exe_path:
            m_data[type::exe_path] = setting_exe_path_t::get(m_file);
            break;
        default:
            log_file("setting_type\n");
            break;
        }
    }
    return;
}

setting_t::~setting_t() {
    for (auto i{ m_data.begin() }; i != m_data.end(); ++i) {
        delete i->second;
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
    member->set_data(std::forward<t_in&&>(in));
    return;
}

auto setting_t::save() -> void {
    m_file.close();
    m_file.open(m_path, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
    version_current.pad(m_file);
    for (auto i{ m_data.begin() }; i != m_data.end(); ++i) {
        pad_integer(m_file, i->first);
        switch (i->first) {
        case type::logfile_path:
            dynamic_cast<setting_logfile_path_t*>(m_data[type::logfile_path])->pad(m_file);
            break;
        case type::source_path:
            dynamic_cast<setting_source_path_t*>(m_data[type::source_path])->pad(m_file);
            break;
        case type::exe_path:
            dynamic_cast<setting_exe_path_t*>(m_data[type::exe_path])->pad(m_file);
            break;
        default:
            log_file("setting_type\n");
            break;
        }
    }
    return;
}

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

auto inform_restore_callback(state_t* state, std::uint64_t previous_state) -> void {
    state->restore_state(previous_state);
    state->remove_state(previous_state);
    return;
}

auto inform_restore(state_t* state, std::wstring info) -> void {
    std::uint64_t previous_state{ state->save_state() };
    state->clear_state();
    state->add_object<text_t>(info, pos_2D{ 0x80, 0x80 }, size_2D{ 0x300, 0x40 }
    , pos_2D{ 0x80, 0x80 }, size_2D{ 0x300, 0x40 }, size_1D{ 0x10 }, color_t{ 1.0f, 1.0f, 1.0f }, alignment_2D{ alignment_x::left, alignment_y::center });
    state->add_object<button_t>(pos_2D{ 0x1C0, 0x340 }, size_2D{ 0x80, 0x40 }, size_1D{ 0x4 }, L"確定", depth_range_t{ 0.0f, 1.0f }
    , std::bind(inform_restore_callback, state, previous_state));
    return;
}

auto get_input_callback(state_t* state, std::wstring* out, std::uint64_t input_id, std::function<void(void)>& callback) -> void {
    *out = state->get_object<text_input_t>(input_id).lock()->get_text();
    callback();
    return;
}

auto get_input(state_t* state, std::wstring prompt, std::wstring* out, std::function<void(void)> callback) -> void {
    state->clear_state();
    state->add_object<text_t>(prompt, pos_2D{ 0x80, 0x80 }, size_2D{ 0x300, 0x40 }
    , pos_2D{ 0x80, 0x80 }, size_2D{ 0x300, 0x40 }, size_1D{ 0x10 }, color_t{ 1.0f, 1.0f, 1.0f }, alignment_2D{ alignment_x::left, alignment_y::center });
    std::uint64_t input_id{ state->add_object<text_input_t>(pos_2D{ 0x80, 0xC0 }, size_2D{ 0x300, 0x270 }, 0.0f
    , size_1D{ 0x20 }, color_t{ 1.0f, 1.0f, 1.0f }, alignment_2D{ alignment_x::left, alignment_y::top }) };
    state->add_object<button_t>(pos_2D{ 0x1C0, 0x340 }, size_2D{ 0x80, 0x40 }, size_1D{ 0x4 }, L"確定", depth_range_t{ 0.0f, 1.0f }
    , std::bind(get_input_callback, state, out, input_id, callback));
    return;
}

auto get_input_restore_callback(state_t* state, std::wstring* out, std::uint64_t previous_state) -> void {
    state->restore_state(previous_state);
    state->remove_state(previous_state);
    return;
}

auto get_input_restore(state_t* state, std::wstring prompt, std::wstring* out) -> void {
    std::uint64_t previous_state{ state->save_state() };
    state->clear_state();
    get_input(state, prompt, out, std::bind(get_input_restore_callback, state, out, previous_state));
    return;
}

class source_t {
private:
    auto check_valid() -> void;
public:
    std::fstream m_file{};
    bool m_valid{ false };
    
    source_t() = default;
    source_t(std::wstring path);

    auto open(std::wstring path) -> void;
    auto close() -> void;
};

auto source_t::check_valid() -> void {
    if (!m_file.fail()) {
        m_valid = true;
    }
    return;
}

source_t::source_t(std::wstring path): m_file{ create_fstream(path) } {
    check_valid();
    return;
}

auto source_t::open(std::wstring path) -> void {
    m_file = create_fstream(path);
    check_valid();
    return;
}

auto source_t::close() -> void {
    m_valid = false;
    m_file.close();
    return;
}

class code_t {
public:
    std::map<std::wstring, source_t> m_source{};

    code_t() = default;
};

auto compile(state_t* state, setting_t* setting, code_t* code) -> void {
    source_t* main_source{ &code->m_source[setting->get_member<setting_source_path_t>()] };
    if (!main_source->m_valid) {
        inform_restore(state, L"原始碼檔案位置不正確");
        return;
    }
    std::ofstream exe{ create_ofstream(setting->get_member<setting_exe_path_t>()) };
    if (exe.fail()) {
        inform_restore(state, L"執行檔輸出位置不正確");
        return;
    }

    std::string entrance{ get_string(main_source->m_file) };

    std::vector<fun> funs{};
    try {
        while (true) {
            global get{ main_source->m_file };
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

    main_source->close();
    exe.close();
    return;
}

auto change_setting() -> void {
    return;
}

auto edit(state_t* state, setting_t* setting) -> void {
    static code_t code{};
    source_t* main_source{ &code.m_source[setting->get_member<setting_source_path_t>()] };
    *main_source = source_t{ setting->get_member<setting_source_path_t>() };
    state->clear_state();
    state->add_object<button_t>(pos_2D{ 0x300, 0x0 }, size_2D{ 0x80, 0x40 }, size_1D{ 0x4 }
    , L"更改設定", depth_range_t{ 0.0f, 1.0f }, std::bind(change_setting));
    state->add_object<button_t>(pos_2D{ 0x380, 0x0 }, size_2D{ 0x80, 0x40 }, size_1D{ 0x4 }
    , L"開始編譯", depth_range_t{ 0.0f, 1.0f }, std::bind(compile, state, setting, &code));
    std::wstring title{};
    if (main_source->m_valid) {
        title = setting->get_member<setting_source_path_t>();
    }
    else {
        title = L"找不到原始碼檔案";
    }
    state->add_object<text_t>(title, pos_2D{ 0x0, 0x0 }, size_2D{ 0x300, 0x40 }
    , pos_2D{ 0x0, 0x0 }, size_2D{ 0x300, 0x40 }, size_1D{ 0x10 }, color_t{ 1.0f, 1.0f, 1.0f }, alignment_2D{ alignment_x::left, alignment_y::center });
    return;
}

auto open_logfile(state_t* state, setting_t* setting) -> void;

auto open_logfile_callback(state_t* state, setting_t* setting, std::wstring* logfile_path) -> void {
    setting->set_member<setting_logfile_path_t>(*logfile_path);
    delete logfile_path;
    open_logfile(state, setting);
    return;
}

auto open_logfile(state_t* state, setting_t* setting) -> void {
    if (!init_logfile(setting->get_member<setting_logfile_path_t>())) {
        std::wstring* logfile_path{ new std::wstring{} };
        get_input(state, L"記錄檔位置：", logfile_path, std::bind(open_logfile_callback, state, setting, logfile_path));
    }
    else {
        edit(state, setting);
    }
    return;
}

auto WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) -> int {
    init_directx();
    engine_t engine{ instance, size_2D{ 0x400, 0x400 } };
    state_t state{ &engine };
    int argc{};
    LPWSTR* argv{ CommandLineToArgvW(GetCommandLineW(), &argc) };
    std::wstring setting_path{};
    if (argc >= 2) {
        setting_path = argv[1];
    }
    if (argc > 2) {
        log_file("argument_count\n");
    }
    setting_t setting{ setting_path };
    open_logfile(&state, &setting);
    while (!message_loop(&engine)) {}

    setting.save();
    log_file("complete\n");
    return 0;
}