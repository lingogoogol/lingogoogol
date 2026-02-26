#ifndef COMPILERCPP_GUI_DIRECTX_DESCRIPTOR_HEAP_IMPL
#define COMPILERCPP_GUI_DIRECTX_DESCRIPTOR_HEAP_IMPL

#include "descriptor_heap_def.hpp"

descriptor_heap_t::descriptor_heap_t(Microsoft::WRL::ComPtr<ID3D12Device> device
, D3D12_DESCRIPTOR_HEAP_TYPE type, std::uint32_t count): m_descriptor_size{ device->GetDescriptorHandleIncrementSize(type) } {
    init1(device, type, count);
    return;
}

auto descriptor_heap_t::init(Microsoft::WRL::ComPtr<ID3D12Device> device
, D3D12_DESCRIPTOR_HEAP_TYPE type, std::uint32_t count) -> void {
    m_descriptor_size = device->GetDescriptorHandleIncrementSize(type);
    init1(device, type, count);
    return;
}

auto descriptor_heap_t::descriptor_handle_CPU_get(std::uint64_t index) const -> D3D12_CPU_DESCRIPTOR_HANDLE {
    D3D12_CPU_DESCRIPTOR_HANDLE handle{ m_interface->GetCPUDescriptorHandleForHeapStart() };
    handle.ptr += m_descriptor_size * index;
    return handle;
}

auto descriptor_heap_t::descriptor_handle_GPU_get(std::uint64_t index) const -> D3D12_GPU_DESCRIPTOR_HANDLE {
    D3D12_GPU_DESCRIPTOR_HANDLE handle{ m_interface->GetGPUDescriptorHandleForHeapStart() };
    handle.ptr += m_descriptor_size * index;
    return handle;
}

auto descriptor_heap_t::create_SRV(Microsoft::WRL::ComPtr<ID3D12Device> device
, const texture_t& texture, std::uint64_t index) -> SRV_t {
    D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
    desc.Format = texture.format_get();
    desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    desc.Texture2D.MostDetailedMip = 0;
    //Don't set MostDetailedMip and ResourceMinLODClamp at the same time.
    //For MostDetailedMip, mips are in the range [MostDetailedMip, MostDetailedMip + MipLevels - 1].
    //For ResourceMinLODClamp, mips are in the range [ResourceMinLODClamp, MipLevels - 1].
    desc.Texture2D.ResourceMinLODClamp = 0.0f;
    //Set MipLevels to UINT_MAX to indicate all the mipmap levels from MostDetailedMip on down to least detailed.
    desc.Texture2D.MipLevels = UINT_MAX;
    desc.Texture2D.PlaneSlice = 0;
    device->CreateShaderResourceView(texture.interface_get().Get(), &desc, descriptor_handle_CPU_get(index));
    SRV_t SRV{};
    SRV.m_descriptor_heap = this;
    SRV.m_descriptor_heap_index = index;
    SRV.m_texture = &texture;
    return SRV;
}

auto descriptor_heap_t::init1(Microsoft::WRL::ComPtr<ID3D12Device> device
, D3D12_DESCRIPTOR_HEAP_TYPE type, std::uint32_t count) -> void {
    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.Type = type;
    desc.NumDescriptors = count;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc.NodeMask = 0;
    hresult(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_interface)));
    return;
}

#endif