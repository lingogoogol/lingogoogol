#ifndef COMPILERCPP_GUI_DIRECTX_ROOT_SIGNATURE
#define COMPILERCPP_GUI_DIRECTX_ROOT_SIGNATURE

#include <vector>

#include "descriptor_table.hpp"

#include "../../lib/header.hpp"

class root_signature_t {
public:
    root_signature_t() = default;
    root_signature_t(D3D12_ROOT_SIGNATURE_FLAGS flag);

    auto flag_set(D3D12_ROOT_SIGNATURE_FLAGS flag) -> void;
    template<typename t_descriptor_table>
    auto descriptor_table_add(t_descriptor_table&& descriptor_table
    , D3D12_SHADER_VISIBILITY shader_visibility) -> std::uint32_t;
    //count: The number of 32bit constants that appear like a single constant buffer.
    auto constant_add(std::uint32_t reg_space, std::uint32_t reg, std::uint32_t count
    , D3D12_SHADER_VISIBILITY shader_visibility) -> std::uint32_t;
    auto descriptor_add(std::uint32_t reg_space, std::uint32_t reg, D3D12_ROOT_PARAMETER_TYPE type
    , D3D12_SHADER_VISIBILITY shader_visibility) -> std::uint32_t;
    auto static_sampler_add(std::uint32_t reg_space, std::uint32_t reg
    , D3D12_SHADER_VISIBILITY shader_visibility) -> void;
    auto serialize(Microsoft::WRL::ComPtr<ID3D12Device> device) -> void;
    auto interface_get() const -> Microsoft::WRL::ComPtr<ID3D12RootSignature>;
private:
    D3D12_ROOT_SIGNATURE_FLAGS m_flag{};
    std::vector<descriptor_table_t> m_descriptor_table{};
    std::vector<D3D12_ROOT_PARAMETER1> m_parameter{};
    std::vector<D3D12_STATIC_SAMPLER_DESC> m_static_sampler{};
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_interface{};
};

root_signature_t::root_signature_t(D3D12_ROOT_SIGNATURE_FLAGS flag): m_flag{ flag } {}

auto root_signature_t::flag_set(D3D12_ROOT_SIGNATURE_FLAGS flag) -> void {
    m_flag = flag;
    return;
}

template<typename t_descriptor_table>
auto root_signature_t::descriptor_table_add(t_descriptor_table&& descriptor_table
, D3D12_SHADER_VISIBILITY shader_visibility) -> std::uint32_t {
    m_descriptor_table.push_back(std::forward<t_descriptor_table&&>(descriptor_table));
    D3D12_ROOT_PARAMETER1 parameter{};
    parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    parameter.DescriptorTable = m_descriptor_table.back().desc_get();
    parameter.ShaderVisibility = shader_visibility;
    m_parameter.push_back(parameter);
    return static_cast<std::uint32_t>(m_parameter.size() - 1);
}

auto root_signature_t::constant_add(std::uint32_t reg_space, std::uint32_t reg, std::uint32_t count
, D3D12_SHADER_VISIBILITY shader_visibility) -> std::uint32_t {
    D3D12_ROOT_PARAMETER1 parameter{};
    parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    parameter.Constants.RegisterSpace = reg_space;
    parameter.Constants.ShaderRegister = reg;
    parameter.Constants.Num32BitValues = count;
    parameter.ShaderVisibility = shader_visibility;
    m_parameter.push_back(parameter);
    return static_cast<std::uint32_t>(m_parameter.size() - 1);
}

auto root_signature_t::descriptor_add(std::uint32_t reg_space, std::uint32_t reg, D3D12_ROOT_PARAMETER_TYPE type
, D3D12_SHADER_VISIBILITY shader_visibility) -> std::uint32_t {
    D3D12_ROOT_PARAMETER1 parameter{};
    parameter.ParameterType = type;
    parameter.Descriptor.RegisterSpace = reg_space;
    parameter.Descriptor.ShaderRegister = reg;
    parameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
    parameter.ShaderVisibility = shader_visibility;
    m_parameter.push_back(parameter);
    return static_cast<std::uint32_t>(m_parameter.size() - 1);
}

auto root_signature_t::static_sampler_add(std::uint32_t reg_space, std::uint32_t reg
, D3D12_SHADER_VISIBILITY shader_visibility) -> void {
    D3D12_STATIC_SAMPLER_DESC desc{};
    desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    desc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    desc.MipLODBias = 0;
    desc.MinLOD = 0.0f;
    desc.MaxLOD = D3D12_FLOAT32_MAX;
    desc.ShaderRegister = reg;
    desc.RegisterSpace = reg_space;
    desc.ShaderVisibility = shader_visibility;
    m_static_sampler.push_back(desc);
    return;
}

auto root_signature_t::serialize(Microsoft::WRL::ComPtr<ID3D12Device> device) -> void {
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc{};
    desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    desc.Desc_1_1.pParameters = m_parameter.data();
    desc.Desc_1_1.NumParameters = static_cast<UINT>(m_parameter.size());
    desc.Desc_1_1.pStaticSamplers = m_static_sampler.data();
    desc.Desc_1_1.NumStaticSamplers = static_cast<UINT>(m_static_sampler.size());
    desc.Desc_1_1.Flags = m_flag;
    Microsoft::WRL::ComPtr<ID3DBlob> blob{};
    Microsoft::WRL::ComPtr<ID3DBlob> error_blob{};
    hresult(D3D12SerializeVersionedRootSignature(&desc, &blob, &error_blob));
    hresult(device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&m_interface)));
    return;
}

auto root_signature_t::interface_get() const -> Microsoft::WRL::ComPtr<ID3D12RootSignature> {
    return m_interface;
}

#endif