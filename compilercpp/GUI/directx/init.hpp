#ifndef COMPILERCPP_GUI_DIRECTX_INIT
#define COMPILERCPP_GUI_DIRECTX_INIT

#include <stacktrace>

#include "../../lib/header.hpp"

auto log_stacktrace(std::stacktrace stack) -> void;
auto handle_device_removed(HRESULT result, Microsoft::WRL::ComPtr<ID3D12Device2> device
, std::stacktrace stack = std::stacktrace::current()) -> bool;
auto hresult(HRESULT result, Microsoft::WRL::ComPtr<ID3D12Device2> device = nullptr
, std::stacktrace stack = std::stacktrace::current()) -> void;
auto FT_error(FT_Error result, std::stacktrace stack = std::stacktrace::current()) -> void;

auto init_directx() -> void;
auto init_freetype() -> void;
auto check_tearing_support(Microsoft::WRL::ComPtr<IDXGIFactory5> factory) -> bool;

auto log_stacktrace(std::stacktrace stack) -> void {
    log_file("  stack:\n");
    for (auto i : stack) {
        log_file("    " + std::to_string(i) + "\n");
    }
    return;
}

auto handle_device_removed(HRESULT result, Microsoft::WRL::ComPtr<ID3D12Device2> device, std::stacktrace stack) -> bool {
    if (result != DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_RESET) {
        return false;
    }
    if (!device) {
        log_file("The device is removed but no device is passed in.\n");
        log_stacktrace(stack);
        return true;
    }
    HRESULT reason{ device->GetDeviceRemovedReason() };
    if (reason != DXGI_ERROR_DEVICE_REMOVED) {
        log_file("The device is removed. reason: " + std::to_string(reason) + "\n");
        log_stacktrace(stack);
        Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedData> DRED_data{};
        hresult(device->QueryInterface(IID_PPV_ARGS(&DRED_data)));
        D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT breadcrumb{};
        hresult(DRED_data->GetAutoBreadcrumbsOutput(&breadcrumb));
        D3D12_DRED_PAGE_FAULT_OUTPUT page_fault{};
        hresult(DRED_data->GetPageFaultAllocationOutput(&page_fault));
        const D3D12_AUTO_BREADCRUMB_NODE* list{ breadcrumb.pHeadAutoBreadcrumbNode };
        while (list) {
            log_file("    name: " + to_string(list->pCommandListDebugNameW));
            //...
            list = list->pNext;
        }
        log_file("  page fault GPU VA: " + std::to_string(page_fault.PageFaultVA));
        const std::function log_obj_info{
            [] (const D3D12_DRED_ALLOCATION_NODE* obj) -> void {
                while (obj) {
                    log_file("    name: " + to_string(obj->ObjectNameW));
                    //...
                    obj = obj->pNext;
                }
            }
        };
        log_file("  existing object:\n");
        log_obj_info(page_fault.pHeadExistingAllocationNode);
        log_file("  recently freed object:\n");
        log_obj_info(page_fault.pHeadRecentFreedAllocationNode);
    }
    //...
    //recreate device and resources
    return true;
}

auto hresult(HRESULT result, Microsoft::WRL::ComPtr<ID3D12Device2> device, std::stacktrace stack) -> void {
    if (result == S_OK) {
        return;
    }
    if (handle_device_removed(result, device)) {
        return;
    }
    log_file("HRESULT != S_OK:\n");
    log_file("  HRESULT: " + std::to_string(result) + "\n");
    log_stacktrace(stack);
    log_file("\n");
    return;
}

auto FT_error(FT_Error result, std::stacktrace stack) -> void {
    if (result == FT_Err_Ok) {
        return;
    }
    log_file("FT_Error != FT_Err_Ok:\n");
    log_file("  FT_Error: " + std::to_string(result) + "\n");
    log_stacktrace(stack);
    log_file("\n");
    return;
}

auto init_directx() -> void {
    DPI_AWARENESS_CONTEXT old_DPI_awareness{ SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) };
    if (old_DPI_awareness == nullptr) {
        throw;
    }
    Microsoft::WRL::ComPtr<ID3D12Debug> debug_interface{};
    Microsoft::WRL::ComPtr<ID3D12Debug1> debug_interface1{};
    hresult(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface)));
    hresult(debug_interface->QueryInterface(IID_PPV_ARGS(&debug_interface1)));
    debug_interface1->EnableDebugLayer();
    debug_interface1->SetEnableGPUBasedValidation(true);
    Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedDataSettings> DRED{};
    hresult(D3D12GetDebugInterface(IID_PPV_ARGS(&DRED)));
    DRED->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
    DRED->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
    DRED->SetWatsonDumpEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
    return;
}

auto init_freetype() -> void {
    FT_Library freetype{};
    FT_Init_FreeType(&freetype);
    return;
}

auto check_tearing_support(Microsoft::WRL::ComPtr<IDXGIFactory5> factory) -> bool {
    BOOL out{};
    factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &out, sizeof out);
    return out;
}

#endif