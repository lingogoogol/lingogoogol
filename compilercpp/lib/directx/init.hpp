#ifndef COMPILERCPP_LIB_DIRECTX_INIT
#define COMPILERCPP_LIB_DIRECTX_INIT

#include "../header/Windows.h"
#include "../header/d3d12.h"
#include "../header/wrl_client.h"

auto init_directx() -> void {
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    Microsoft::WRL::ComPtr<ID3D12Debug> debug_interface{};
    D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface));
    debug_interface->EnableDebugLayer();
    return;
}

#endif