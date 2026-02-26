#ifndef COMPILERCPP_GUI_DIRECTX_COMMAND_LIST
#define COMPILERCPP_GUI_DIRECTX_COMMAND_LIST

#include <array>

#include "../../lib/header.hpp"

auto push_transition_barrier(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list
, Microsoft::WRL::ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) -> void {
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    command_list->ResourceBarrier(1, &barrier);
    return;
}

auto clear_RT(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list
, D3D12_CPU_DESCRIPTOR_HANDLE RTV_handle, const std::array<FLOAT, 4>& color) -> void {
    command_list->ClearRenderTargetView(RTV_handle, color.data(), 0, nullptr);
    return;
}

#endif