#ifndef COMPILERCPP_GUI_DIRECTX_DESCRIPTOR_HEAP_DEF
#define COMPILERCPP_GUI_DIRECTX_DESCRIPTOR_HEAP_DEF

#include "misc.hpp"
#include "texture.hpp"
#include "SRV_def.hpp"

#include "../../lib/header.hpp"

class descriptor_heap_t {
public:
    descriptor_heap_t() = default;
    descriptor_heap_t(Microsoft::WRL::ComPtr<ID3D12Device> device
    , D3D12_DESCRIPTOR_HEAP_TYPE type, std::uint32_t count, std::string name);
    auto init(Microsoft::WRL::ComPtr<ID3D12Device> device
    , D3D12_DESCRIPTOR_HEAP_TYPE type, std::uint32_t count, std::string name) -> void;

    auto interface_get() const -> Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>;
    auto descriptor_handle_CPU_get(std::uint64_t index) const -> D3D12_CPU_DESCRIPTOR_HANDLE;
    auto descriptor_handle_GPU_get(std::uint64_t index) const -> D3D12_GPU_DESCRIPTOR_HANDLE;

    auto create_SRV(Microsoft::WRL::ComPtr<ID3D12Device> device
    , const texture_t& texture, std::uint64_t index) -> SRV_t;
    auto create_SRV_null(Microsoft::WRL::ComPtr<ID3D12Device> device, std::uint64_t index) -> void;
private:
    //Update m_descriptor_heap without changing m_descriptor_size.
    auto init1(Microsoft::WRL::ComPtr<ID3D12Device> device
    , D3D12_DESCRIPTOR_HEAP_TYPE type, std::uint32_t count) -> void;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_interface{};
    std::uint32_t m_descriptor_size{};
    std::string m_name{};
};

#endif