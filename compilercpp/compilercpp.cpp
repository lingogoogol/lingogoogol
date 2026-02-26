#include <iostream>
#include <fstream>
#include <cstdint>
#include <string>
#include <array>
#include <vector>

#define UNICODE

#include "lib/header/Windows.h"
#include "lib/header/wrl_client.h"
#include "lib/header/d3d12.h"
#include "lib/header/dxgi1_6.h"
#include "lib/header/shellapi.h"

Microsoft::WRL::ComPtr<ID3D12InfoQueue> info_queue{};

#include "lib/.hpp"
#include "out/.hpp"
#include "global/.hpp"
#include "stmt/.hpp"
#include "expr/.hpp"
#include "instr/.hpp"

template<typename t_in>
auto log_ref_count(t_in in) -> void {
    in->AddRef();
    log_console(std::to_string(in->Release()));
    return;
}

auto compile(std::istream& source, std::ostream& exe) -> void {
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
                log_file("global type\r\n");
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
    return;
}

constexpr inline LONG window_width{ 0x400 };
constexpr inline LONG window_height{ 0x400 };
constexpr inline UINT buffer_count{ 0x2 };
const inline std::wstring window_name{ L"compiler" };

struct window_state {
    bool m_initialized{ false };
    bool m_tearing_supported{};
    Microsoft::WRL::ComPtr<ID3D12InfoQueue> m_info_queue{};
    Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swap_chain{};
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_RT{};
    command_queue m_command_queue{};
    std::vector<UINT64> m_buffer_fence_value{};
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RTV_heap{};
    UINT m_RTV_size{};
    UINT m_current_buffer_index{};
};

auto CALLBACK WndProc(HWND window, UINT message, WPARAM param_1, LPARAM param_2) -> LRESULT {
    window_state* state{ reinterpret_cast<window_state*>(GetWindowLongPtrW(window, GWLP_USERDATA)) };
    if (!state || !state->m_initialized) {
        return DefWindowProcW(window, message, param_1, param_2);
    }
    switch (message) {
    case WM_PAINT: {
        log_info_queue(info_queue);
        Microsoft::WRL::ComPtr<ID3D12Resource> RT_current{ state->m_RT[state->m_current_buffer_index] };
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list{ state->m_command_queue.create_list() };
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = RT_current.Get();
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        command_list->ResourceBarrier(1, &barrier);
        D3D12_CPU_DESCRIPTOR_HANDLE RTV_handle{};
        RTV_handle.ptr = state->m_RTV_heap->GetCPUDescriptorHandleForHeapStart().ptr + state->m_RTV_size * state->m_current_buffer_index;
        FLOAT clear_color[4]{ 0.0f, 1.0f, 0.0f, 1.0f };
        command_list->ClearRenderTargetView(RTV_handle, clear_color, 0, nullptr);
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        command_list->ResourceBarrier(1, &barrier);
        state->m_command_queue.execute_list(command_list);
        state->m_buffer_fence_value[state->m_current_buffer_index] = state->m_command_queue.set_fence();
        state->m_swap_chain->Present(0, state->m_tearing_supported ? DXGI_PRESENT_ALLOW_TEARING : 0);
        state->m_current_buffer_index = state->m_swap_chain->GetCurrentBackBufferIndex();
        state->m_command_queue.wait_fence(state->m_buffer_fence_value[state->m_current_buffer_index]);
        return 0;
    }
    case WM_DESTROY: {
        PostQuitMessage(0);
        return 0;
    }
    default: {
        return DefWindowProcW(window, message, param_1, param_2);
    }
    }
}

auto check_tearing_support(Microsoft::WRL::ComPtr<IDXGIFactory5> factory) -> bool {
    bool out{};
    factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &out, sizeof out);
    return out;
}

auto WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR arg, int) -> int {
    int argc{};
    LPWSTR* argv{ CommandLineToArgvW(arg, &argc) };
    if (argc != 3) {
        log_file("command_line_argument_count");
    }

    init_logfile(argv[2]);
    init_directx();
    Microsoft::WRL::ComPtr<IDXGIFactory5> factory{ create_factory() };
    window_state state{};
    state.m_tearing_supported = check_tearing_support(factory);
    ATOM window_class{ create_window_class(L"window_class", &WndProc, instance
    , reinterpret_cast<HICON>(LoadImageW(NULL, IDI_APPLICATION, IMAGE_ICON
    , GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_SHARED))
    , reinterpret_cast<HICON>(LoadImageW(NULL, IDI_APPLICATION, IMAGE_ICON
    , GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED))
    , reinterpret_cast<HCURSOR>(LoadImageW(NULL, IDC_ARROW, IMAGE_CURSOR
    , GetSystemMetrics(SM_CXCURSOR), GetSystemMetrics(SM_CYCURSOR), LR_SHARED))) };
    HWND window{ create_window(window_class, L"compilercpp"
    , WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, window_width, window_height, instance) };
    SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&state));
    Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter{ create_adapter(factory) };
    Microsoft::WRL::ComPtr<ID3D12Device2> device{ create_device(adapter) };
    state.m_info_queue = create_info_queue(device);
    info_queue = state.m_info_queue;
    state.m_command_queue.init(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    state.m_swap_chain = state.m_command_queue.create_swap_chain(factory, window, window_width
    , window_height , DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_USAGE_RENDER_TARGET_OUTPUT
    , buffer_count, state.m_tearing_supported);
    state.m_RTV_heap = create_V_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, buffer_count);
    state.m_RTV_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    state.m_RT = create_RT(device, state.m_swap_chain, state.m_RTV_heap, state.m_RTV_size, buffer_count);
    state.m_current_buffer_index = state.m_swap_chain->GetCurrentBackBufferIndex();
    state.m_buffer_fence_value.resize(buffer_count);
    state.m_initialized = true;
    ShowWindow(window, SW_SHOW);
    MSG message{};
    BOOL message_get_result{};
    while ((message_get_result = GetMessageW(&message, NULL, 0, 0))) {
        if (message_get_result == -1) {
            log_file("message_get\r\n");
        }
        DispatchMessageW(&message);
    }
    state.m_command_queue.flush();

    std::ifstream source{ create_ifstream(argv[0], std::ios_base::in | std::ios_base::binary) };
    std::ofstream exe{ create_ofstream(argv[1], std::ios_base::out
    | std::ios_base::binary | std::ios_base::trunc) };
    compile(source, exe);

    source.close();
    exe.close();
    log_info_queue(info_queue);
    log_file("complete\r\n");
    return 0;
}