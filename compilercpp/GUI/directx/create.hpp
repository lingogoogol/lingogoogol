#ifndef COMPILERCPP_GUI_DIRECTX_CREATE
#define COMPILERCPP_GUI_DIRECTX_CREATE

#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <fstream>

#include "../../lib/header.hpp"
#include "../../lib/stu.hpp"
#include "../../lib/io.hpp"

auto create_factory(bool debugging) -> Microsoft::WRL::ComPtr<IDXGIFactory5> {
    Microsoft::WRL::ComPtr<IDXGIFactory5> out{};
    hresult(CreateDXGIFactory2(debugging ? DXGI_CREATE_FACTORY_DEBUG : 0, IID_PPV_ARGS(&out)));
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

auto create_window(ATOM window_class, const std::wstring& name
, DWORD style, LONG width, LONG height, HINSTANCE instance, pos_2D* window_pos) -> HWND {
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
    window_pos->x = (width_screen - width_adjusted) / 2;
    window_pos->y = (height_screen - height_adjusted) / 2;
    return CreateWindowW(reinterpret_cast<LPCWSTR>(window_class), name.data()
    , style, static_cast<int>(window_pos->x), static_cast<int>(window_pos->y)
    , width_adjusted, height_adjusted, NULL, NULL, instance, NULL);
}

auto create_adapter(Microsoft::WRL::ComPtr<IDXGIFactory5> factory)
-> Microsoft::WRL::ComPtr<IDXGIAdapter4> {
    Microsoft::WRL::ComPtr<IDXGIAdapter4> out{};
    for (UINT i{ 0 }; true; ++i) {
        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter1{};
        if (factory->EnumAdapters1(i, &adapter1) == DXGI_ERROR_NOT_FOUND) {
            break;
        }
        DXGI_ADAPTER_DESC1 description{};
        hresult(adapter1->GetDesc1(&description));
        if (SUCCEEDED(D3D12CreateDevice(out.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr))) {
            hresult(adapter1.As(&out));
            break;
        }
    }
    return out;
}

auto create_device(Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter, std::string name) -> Microsoft::WRL::ComPtr<ID3D12Device2> {
    Microsoft::WRL::ComPtr<ID3D12Device2> out{};
    hresult(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&out)));
    D3D12_set_name(out, name);
    return out;
}

auto create_info_queue(Microsoft::WRL::ComPtr<ID3D12Device2> device) -> Microsoft::WRL::ComPtr<ID3D12InfoQueue> {
    Microsoft::WRL::ComPtr<ID3D12InfoQueue> out{};
    hresult(device.As(&out));
    D3D12_INFO_QUEUE_FILTER info_queue_filter{};
    std::vector<D3D12_MESSAGE_SEVERITY> info_queue_filter_severity{};
    if (false) {
        info_queue_filter_severity.push_back(D3D12_MESSAGE_SEVERITY_INFO);
    }
    info_queue_filter.DenyList.NumSeverities = static_cast<UINT>(info_queue_filter_severity.size());
    info_queue_filter.DenyList.pSeverityList = info_queue_filter_severity.data();
    hresult(out->PushStorageFilter(&info_queue_filter));
    return out;
}

auto create_V_heap(Microsoft::WRL::ComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT count, std::string name) -> Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> {
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> out{};
    D3D12_DESCRIPTOR_HEAP_DESC description{};
    description.Type = type;
    description.NumDescriptors = count;
    description.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    description.NodeMask = 0;
    hresult(device->CreateDescriptorHeap(&description, IID_PPV_ARGS(&out)));
    D3D12_set_name(out, name);
    return out;
}

auto create_RT(Microsoft::WRL::ComPtr<IDXGISwapChain4> swap_chain, UINT count) -> std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> {
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> out{};
    out.resize(count);
    for (UINT i{ 0 }; i < count; ++i) {
        hresult(swap_chain->GetBuffer(i, IID_PPV_ARGS(&(out[i]))));
        D3D12_set_name(out[i], "swap chain's buffer[index = " + std::to_string(i) + "]");
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

auto create_upload_heap_property() -> D3D12_HEAP_PROPERTIES {
    D3D12_HEAP_PROPERTIES out{};
    out.Type = D3D12_HEAP_TYPE_UPLOAD;
    //When Type is not D3D12_HEAP_TYPE_CUSTOM, CPUPageProperty and MemoryPoolPreference must be ..._UNKNOWN.
    out.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    out.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    out.CreationNodeMask = 0;
    out.VisibleNodeMask = 0;
    return out;
}

auto create_default_heap_property() -> D3D12_HEAP_PROPERTIES {
    D3D12_HEAP_PROPERTIES out{};
    out.Type = D3D12_HEAP_TYPE_DEFAULT;
    //When Type is not D3D12_HEAP_TYPE_CUSTOM, CPUPageProperty and MemoryPoolPreference must be ..._UNKNOWN.
    out.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    out.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    out.CreationNodeMask = 0;
    out.VisibleNodeMask = 0;
    return out;
}

auto create_resource_desc_buffer(std::uint64_t size, D3D12_RESOURCE_FLAGS flags) -> D3D12_RESOURCE_DESC {
    D3D12_RESOURCE_DESC desc{};
    //When Dimension is _BUFFER, ...
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    //Alignment must be 64KB (D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT) or 0, which is effectively 64KB.
    desc.Alignment = 0;
    desc.Width = size;
    //Height, DepthOrArraySize, and MipLevels must be 1.
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    //Format must be DXGI_FORMAT_UNKNOWN.
    desc.Format = DXGI_FORMAT_UNKNOWN;
    //SampleDesc.Count must be 1 and Quality must be 0.
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    //Layout must be D3D12_TEXTURE_LAYOUT_ROW_MAJOR.
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = flags;
    return desc;
}

auto create_resource_upload(Microsoft::WRL::ComPtr<ID3D12Device> device, std::uint64_t size, std::string name)
-> Microsoft::WRL::ComPtr<ID3D12Resource> {
    Microsoft::WRL::ComPtr<ID3D12Resource> resource{};
    D3D12_HEAP_PROPERTIES upload_heap_properties{ create_upload_heap_property() };
    D3D12_RESOURCE_DESC upload_resource_desc{ create_resource_desc_buffer(size, D3D12_RESOURCE_FLAG_NONE) };
    //Resources in the upload heap must be created with D3D12_RESOURCE_STATE_GENERIC_READ and cannot be changed away from this.
    hresult(device->CreateCommittedResource(&upload_heap_properties, D3D12_HEAP_FLAG_NONE
    , &upload_resource_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource)));
    D3D12_set_name(resource, name);
    return resource;
}

//For buffer.
auto create_copy_location(Microsoft::WRL::ComPtr<ID3D12Resource> resource
, const UINT& subresource_index) -> D3D12_TEXTURE_COPY_LOCATION {
    D3D12_TEXTURE_COPY_LOCATION out{};
    out.pResource = resource.Get();
    out.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    out.SubresourceIndex = subresource_index;
    return out;
}

//For texture.
auto create_copy_location(Microsoft::WRL::ComPtr<ID3D12Resource> resource
, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint) -> D3D12_TEXTURE_COPY_LOCATION {
    D3D12_TEXTURE_COPY_LOCATION out{};
    out.pResource = resource.Get();
    out.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    out.PlacedFootprint = footprint;
    return out;
}

#endif