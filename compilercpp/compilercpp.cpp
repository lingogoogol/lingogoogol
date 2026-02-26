#include <iostream>
#include <fstream>
#include <cstdint>
#include <string>
#include <array>

#define UNICODE
#include <Windows.h>
#include <shellapi.h>
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#undef min
#undef max

#include "lib/.hpp"
#include "out/.hpp"
#include "global/.hpp"
#include "stmt/.hpp"
#include "expr/.hpp"
#include "instr/.hpp"

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
                throw internal_error{ "global type" };
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

auto CALLBACK WndProc(HWND window, UINT message, WPARAM param_1, LPARAM param_2) -> LRESULT {
    return DefWindowProcW(window, message, param_1, param_2);
}

constexpr inline LONG window_width{ 0x100 };
constexpr inline LONG window_height{ 0x100 };
constexpr inline UINT buffer_count{ 0x2 };
const inline std::wstring window_name{ L"compiler" };
bool initialized{ false };

//#define EXCEPTION_HANDLING_NO

auto WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR arg, int) -> int {
#ifndef EXCEPTION_HANDLING_NO
    try {
#endif
        int argc{};
        LPWSTR* argv{ CommandLineToArgvW(arg, &argc) };
        if (argc != 3) {
            throw error{ error::command_line_argument };
        }

        init_logfile(argv[2]);
        SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        Microsoft::WRL::ComPtr<ID3D12Debug> debug_interface{};
        D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface));
        debug_interface->EnableDebugLayer();
        Microsoft::WRL::ComPtr<IDXGIFactory5> factory{};
        CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory));
        bool allow_tearing{ false };
        factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allow_tearing, sizeof allow_tearing);
        WNDCLASSEXW window_class{};
        std::wstring window_class_name{ L"window_class" };
        window_class.cbSize = sizeof window_class;
        window_class.style = CS_HREDRAW | CS_VREDRAW;
        window_class.lpfnWndProc = &WndProc;
        window_class.cbClsExtra = 0;
        window_class.cbWndExtra = 0;
        window_class.hInstance = instance;
        window_class.hIcon = (HICON)(LoadImageW(NULL, IDI_APPLICATION, IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_SHARED));
        window_class.hCursor = (HCURSOR)(LoadImageW(NULL, IDC_ARROW, IMAGE_CURSOR, GetSystemMetrics(SM_CXCURSOR), GetSystemMetrics(SM_CYCURSOR), LR_SHARED));
        window_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        window_class.lpszMenuName = NULL;
        window_class.lpszClassName = window_class_name.data();
        window_class.hIconSm = (HICON)(LoadImageW(NULL, IDI_APPLICATION, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED));
        ATOM window_class_atom{ RegisterClassExW(&window_class) };
        RECT window_rect{ 0, 0, window_width, window_height };
        constexpr DWORD window_style{ WS_OVERLAPPEDWINDOW };
        AdjustWindowRect(&window_rect, window_style, false);
        HWND window{ CreateWindowW((LPCWSTR)window_class_atom, window_name.data(), window_style, 0, 0, window_width, window_height, NULL, NULL, instance, NULL) };
        Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter{};
        SIZE_T max_dedicated_video_memory{ 0 };
        for (UINT i{ 0 }; true; ++i) {
            Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter1{};
            if (factory->EnumAdapters1(i, &adapter1) == DXGI_ERROR_NOT_FOUND) {
                break;
            }
            DXGI_ADAPTER_DESC1 adapter_description{};
            adapter1->GetDesc1(&adapter_description);
            if (adapter_description.DedicatedVideoMemory > max_dedicated_video_memory
            && SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr))) {
                max_dedicated_video_memory = adapter_description.DedicatedVideoMemory;
                adapter1.As(&adapter);
            }
        }
        Microsoft::WRL::ComPtr<ID3D12Device2> device{};
        D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device));
        Microsoft::WRL::ComPtr<ID3D12InfoQueue> info_queue{};
        device.As(&info_queue);
        info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
        D3D12_INFO_QUEUE_FILTER info_queue_filter{};
        std::array info_queue_filter_severity{ D3D12_MESSAGE_SEVERITY_INFO };
        info_queue_filter.DenyList.NumSeverities = info_queue_filter_severity.size();
        info_queue_filter.DenyList.pSeverityList = info_queue_filter_severity.data();
        info_queue->PushStorageFilter(&info_queue_filter);
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue{};
        D3D12_COMMAND_QUEUE_DESC command_queue_description{};
        command_queue_description.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        command_queue_description.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        command_queue_description.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        command_queue_description.NodeMask = 0;
        device->CreateCommandQueue(&command_queue_description, IID_PPV_ARGS(&command_queue));
        Microsoft::WRL::ComPtr<IDXGISwapChain4> swap_chain{};
        DXGI_SWAP_CHAIN_DESC1 swap_chain_description{};
        swap_chain_description.Width = window_width;
        swap_chain_description.Height = window_height;
        swap_chain_description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swap_chain_description.Stereo = false;
        swap_chain_description.SampleDesc = { 1, 0 };
        swap_chain_description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_chain_description.BufferCount = buffer_count;
        swap_chain_description.Scaling = DXGI_SCALING_NONE;
        swap_chain_description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swap_chain_description.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        swap_chain_description.Flags = allow_tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
        Microsoft::WRL::ComPtr<IDXGISwapChain1> swap_chain1{};
        factory->CreateSwapChainForHwnd(command_queue.Get(), window, &swap_chain_description, nullptr, nullptr, &swap_chain1);
        swap_chain1.As(&swap_chain);
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> RTV_heap{};
        D3D12_DESCRIPTOR_HEAP_DESC RTV_heap_description{};
        RTV_heap_description.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        RTV_heap_description.NumDescriptors = buffer_count;
        RTV_heap_description.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        RTV_heap_description.NodeMask = 0;
        device->CreateDescriptorHeap(&RTV_heap_description, IID_PPV_ARGS(&RTV_heap));
        UINT RTV_size{ device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) };
        D3D12_CPU_DESCRIPTOR_HANDLE RTV{ RTV_heap->GetCPUDescriptorHandleForHeapStart() };
        std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, buffer_count> RT{};
        std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, buffer_count> command_allocator{};
        for (UINT i{ 0 }; i < buffer_count; ++i) {
            swap_chain->GetBuffer(i, IID_PPV_ARGS(&(RT[i])));
            device->CreateRenderTargetView(RT[i].Get(), nullptr, RTV);
            RTV.ptr += RTV_size;
            device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&(command_allocator[i])));
        }
        UINT current_buffer_index{ swap_chain->GetCurrentBackBufferIndex() };
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list{};
        device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, command_allocator[current_buffer_index].Get(), nullptr, IID_PPV_ARGS(&command_list));
        Microsoft::WRL::ComPtr<ID3D12Fence> fence{};
        UINT64 fence_value{ 0 };
        device->CreateFence(fence_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
        HANDLE event{};
        CreateEventW(nullptr, false, false, nullptr);
        initialized = true;
        ShowWindow(window, SW_SHOW);
        MSG message{};
        BOOL message_get_result{};
        while (message_get_result = GetMessageW(&message, window, 0, 0)) {
            if (message_get_result == -1) {
                throw internal_error{ "message_get" };
            }
            DispatchMessageW(&message);
        }

        std::ifstream source{ create_ifstream(argv[0], std::ios_base::in | std::ios_base::binary) };
        std::ofstream exe{ create_ofstream(argv[1], std::ios_base::out | std::ios_base::binary | std::ios_base::trunc) };
        compile(source, exe);

        source.close();
        exe.close();
        log("complete\r\n");
        return 0;
#ifndef EXCEPTION_HANDLING_NO
    }
    catch (std::exception& e) {
        log("error: ");
        log(e.what());
        log("\r\n");
        return 1;
    }
    catch (...) {
        log("unknown error\r\n");
        return 1;
    }
#endif
}