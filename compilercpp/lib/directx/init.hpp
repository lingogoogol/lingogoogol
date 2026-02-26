#ifndef COMPILERCPP_LIB_DIRECTX_INIT
#define COMPILERCPP_LIB_DIRECTX_INIT

#include "../header/Windows.h"
#include "../header/d3d12.h"
#include "../header/wrl_client.h"
#include "../header/dxgi1_6.h"

auto init_directx() -> void {
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    Microsoft::WRL::ComPtr<ID3D12Debug> debug_interface{};
    D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface));
    debug_interface->EnableDebugLayer();
    return;
}

auto check_tearing_support(Microsoft::WRL::ComPtr<IDXGIFactory5> factory) -> bool {
    bool out{};
    factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &out, sizeof out);
    return out;
}

#endif