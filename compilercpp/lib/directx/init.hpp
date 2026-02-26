#ifndef COMPILERCPP_LIB_DIRECTX_INIT
#define COMPILERCPP_LIB_DIRECTX_INIT

#include "../other/header.hpp"

auto init_directx() -> void {
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    Microsoft::WRL::ComPtr<ID3D12Debug> debug_interface{};
    D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface));
    debug_interface->EnableDebugLayer();
    return;
}

auto check_tearing_support(Microsoft::WRL::ComPtr<IDXGIFactory5> factory) -> bool {
    BOOL out{};
    factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &out, sizeof out);
    return out;
}

#endif