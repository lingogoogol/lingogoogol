#ifndef COMPILERCPP_LIB_DIRECTX_CREATE
#define COMPILERCPP_LIB_DIRECTX_CREATE

#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <fstream>

#include "../header.hpp"

auto create_factory(bool debugging) -> Microsoft::WRL::ComPtr<IDXGIFactory5> {
    Microsoft::WRL::ComPtr<IDXGIFactory5> out{};
    CreateDXGIFactory2(debugging ? DXGI_CREATE_FACTORY_DEBUG : 0, IID_PPV_ARGS(&out));
    return out;
}

auto create_window_class(const std::wstring& name, WNDPROC windows_process, HINSTANCE instance, HICON icon, HICON small_icon, HCURSOR cursor) -> ATOM {
    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof window_class;
    window_class.style = 0;
    window_class.lpfnWndProc = windows_process;
    window_class.cbClsExtra = 0;
    window_class.cbWndExtra = 0;
    window_class.hInstance = instance;
    window_class.hIcon = icon;
    window_class.hCursor = cursor;
    window_class.hbrBackground = (HBRUSH)nullptr;
    window_class.lpszMenuName = NULL;
    window_class.lpszClassName = name.data();
    window_class.hIconSm = small_icon;
    return RegisterClassExW(&window_class);
}

auto create_window(Microsoft::WRL::ComPtr<IDXGIFactory5> factory, ATOM window_class, const std::wstring& name
, DWORD style, LONG width, LONG height, HINSTANCE instance) -> HWND {
    RECT rect{ 0, 0, width, height };
    AdjustWindowRect(&rect, style, false);
    int width_adjusted{ rect.right - rect.left };
    int height_adjusted{ rect.bottom - rect.top };
    POINT origin{ 0, 0 };
    HMONITOR primary_monitor{ MonitorFromPoint(origin, MONITOR_DEFAULTTOPRIMARY) };
    MONITORINFO monitor_info{};
    monitor_info.cbSize = sizeof monitor_info;
    GetMonitorInfoW(primary_monitor, &monitor_info);
    int width_screen{ monitor_info.rcWork.right - monitor_info.rcWork.left };
    int height_screen{ monitor_info.rcWork.bottom - monitor_info.rcWork.top };
    return CreateWindowW((LPCWSTR)window_class, name.data(), style, (width_screen - width_adjusted) / 2
    , (height_screen - height_adjusted) / 2, width_adjusted, height_adjusted, NULL, NULL, instance, NULL);
}

auto create_adapter(Microsoft::WRL::ComPtr<IDXGIFactory5> factory)
-> Microsoft::WRL::ComPtr<IDXGIAdapter4> {
    Microsoft::WRL::ComPtr<IDXGIAdapter4> out{};
    SIZE_T max_dedicated_video_memory{ 0 };
    for (UINT i{ 0 }; true; ++i) {
        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter1{};
        if (factory->EnumAdapters1(i, &adapter1) == DXGI_ERROR_NOT_FOUND) {
            break;
        }
        DXGI_ADAPTER_DESC1 description{};
        adapter1->GetDesc1(&description);
        if (description.DedicatedVideoMemory > max_dedicated_video_memory
        && SUCCEEDED(D3D12CreateDevice(out.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr))) {
            max_dedicated_video_memory = description.DedicatedVideoMemory;
            adapter1.As(&out);
        }
    }
    return out;
}

auto create_device(Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter) -> Microsoft::WRL::ComPtr<ID3D12Device2> {
    Microsoft::WRL::ComPtr<ID3D12Device2> out{};
    D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&out));
    return out;
}

auto create_info_queue(Microsoft::WRL::ComPtr<ID3D12Device2> device) -> Microsoft::WRL::ComPtr<ID3D12InfoQueue> {
    Microsoft::WRL::ComPtr<ID3D12InfoQueue> out{};
    device.As(&out);
    D3D12_INFO_QUEUE_FILTER info_queue_filter{};
    std::array info_queue_filter_severity{ D3D12_MESSAGE_SEVERITY_INFO };
    info_queue_filter.DenyList.NumSeverities = static_cast<UINT>(info_queue_filter_severity.size());
    info_queue_filter.DenyList.pSeverityList = info_queue_filter_severity.data();
    out->PushStorageFilter(&info_queue_filter);
    return out;
}

auto create_V_heap(Microsoft::WRL::ComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT count) -> Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> {
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> out{};
    D3D12_DESCRIPTOR_HEAP_DESC description{};
    description.Type = type;
    description.NumDescriptors = count;
    description.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    description.NodeMask = 0;
    device->CreateDescriptorHeap(&description, IID_PPV_ARGS(&out));
    return out;
}

auto create_RT(Microsoft::WRL::ComPtr<IDXGISwapChain4> swap_chain, UINT count) -> std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> {
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> out{};
    out.resize(count);
    for (UINT i{ 0 }; i < count; ++i) {
        swap_chain->GetBuffer(i, IID_PPV_ARGS(&(out[i])));
    }
    return out;
}

auto create_RTV(Microsoft::WRL::ComPtr<ID3D12Device2> device, std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> RT
, UINT RTV_size, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> RTV_heap) -> void {
    D3D12_CPU_DESCRIPTOR_HANDLE RTV{ RTV_heap->GetCPUDescriptorHandleForHeapStart() };
    for (UINT i{ 0 }; i < RT.size(); ++i) {
        device->CreateRenderTargetView(RT[i].Get(), nullptr, RTV);
        RTV.ptr += RTV_size;
    }
    return;
}

auto create_V_handle(D3D12_CPU_DESCRIPTOR_HANDLE start, SIZE_T V_size, SIZE_T index) -> D3D12_CPU_DESCRIPTOR_HANDLE {
    D3D12_CPU_DESCRIPTOR_HANDLE out{};
    out.ptr = start.ptr + V_size * index;
    return out;
}

auto create_input_element(const std::string& semantic_name, DXGI_FORMAT format, UINT slot, UINT instance_count) -> D3D12_INPUT_ELEMENT_DESC {
    D3D12_INPUT_ELEMENT_DESC out{};
    out.SemanticName = semantic_name.data();
    out.SemanticIndex = 0;
    out.Format = format;
    out.InputSlot = slot;
    out.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    out.InstanceDataStepRate = instance_count;
    return out;
}

auto create_shader_bytecode(std::string path, std::string& code) -> D3D12_SHADER_BYTECODE {
    std::ifstream file{ path, std::ios_base::in | std::ios_base::binary };
    auto start{ file.tellg() };
    file.seekg(0, std::ios_base::end);
    auto end{ file.tellg() };
    auto size{ end - start };
    code.resize(size);
    file.seekg(0, std::ios_base::beg);
    file.read(code.data(), size);
    D3D12_SHADER_BYTECODE out{};
    out.pShaderBytecode = code.data();
    out.BytecodeLength = code.size();
    return out;
}

auto create_upload_heap_property() -> D3D12_HEAP_PROPERTIES {
    D3D12_HEAP_PROPERTIES out{};
    out.Type = D3D12_HEAP_TYPE_UPLOAD;
    out.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    out.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    out.CreationNodeMask = 0;
    out.VisibleNodeMask = 0;
    return out;
}

auto create_default_heap_property() -> D3D12_HEAP_PROPERTIES {
    D3D12_HEAP_PROPERTIES out{};
    out.Type = D3D12_HEAP_TYPE_DEFAULT;
    out.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    out.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    out.CreationNodeMask = 0;
    out.VisibleNodeMask = 0;
    return out;
}

#endif