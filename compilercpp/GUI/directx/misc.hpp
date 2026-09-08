#ifndef COMPILERCPP_GUI_DIRECTX_MISC
#define COMPILERCPP_GUI_DIRECTX_MISC

#include "../../lib/header.hpp"

auto D3D12_set_name(Microsoft::WRL::ComPtr<ID3D12Object> obj, std::string name) -> void;
auto hresult(HRESULT result) -> void;
//Include a check for the device removed error when responding to window size changes and after present.
auto handle_device_removed(HRESULT result, Microsoft::WRL::ComPtr<ID3D12Device2> device) -> bool;
auto log_blob(Microsoft::WRL::ComPtr<ID3DBlob> in) -> void;

auto D3D12_set_name(Microsoft::WRL::ComPtr<ID3D12Object> obj, std::string name) -> void {
    obj->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(name.size()), name.data());
    return;
}

auto hresult(HRESULT result) -> void {
    if (result == S_OK) {
        return;
    }
    log_file("HRESULT != S_OK:\n");
    log_file("  HRESULT: " + std::to_string(result) + "\n");
    log_file("\n");
    return;
}

auto handle_device_removed(HRESULT result, Microsoft::WRL::ComPtr<ID3D12Device2> device) -> bool {
    if (result != DXGI_ERROR_DEVICE_REMOVED || result != DXGI_ERROR_DEVICE_RESET) {
        return false;
    }
    if (!device) {
        log_file("The device is removed but no device is passed in.\n");
        return true;
    }
    HRESULT reason{ device->GetDeviceRemovedReason() };
    log_file("The device is removed. reason: " + std::to_string(reason) + "\n");
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
    //...
    //recreate device and resources
    return true;
}

auto log_blob(Microsoft::WRL::ComPtr<ID3DBlob> in) -> void {
    for (SIZE_T i{ 0 }; i < in->GetBufferSize(); ++i) {
        log_file(std::string{ *(reinterpret_cast<const char*>(in->GetBufferPointer()) + i) });
    }
    return;
}

#endif