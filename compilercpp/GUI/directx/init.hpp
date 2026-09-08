#ifndef COMPILERCPP_GUI_DIRECTX_INIT
#define COMPILERCPP_GUI_DIRECTX_INIT

#include "misc.hpp"

#include "../../lib/header.hpp"

auto init_directx() -> void;
auto check_tearing_support(Microsoft::WRL::ComPtr<IDXGIFactory5> factory) -> bool;

auto init_directx() -> void {
    DPI_AWARENESS_CONTEXT old_DPI_awareness{ SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) };
    if (old_DPI_awareness == nullptr) {
        throw;
    }
#ifndef NDEBUG
    Microsoft::WRL::ComPtr<ID3D12Debug> debug_interface{};
    Microsoft::WRL::ComPtr<ID3D12Debug1> debug_interface1{};
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface)))
	&& SUCCEEDED(debug_interface->QueryInterface(IID_PPV_ARGS(&debug_interface1)))) {
		debug_interface1->EnableDebugLayer();
		debug_interface1->SetEnableGPUBasedValidation(true);
	}
    Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedDataSettings> DRED{};
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&DRED)))) {
		DRED->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
		DRED->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
		DRED->SetWatsonDumpEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
	}
#endif
    return;
}

auto check_tearing_support(Microsoft::WRL::ComPtr<IDXGIFactory5> factory) -> bool {
    BOOL out{};
    hresult(factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &out, sizeof out));
    return out;
}

#endif
