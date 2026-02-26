#ifndef COMPILERCPP_GUI_DIRECTX_INPUT_LAYOUT
#define COMPILERCPP_GUI_DIRECTX_INPUT_LAYOUT

#include <vector>

#include "../../lib/header.hpp"

class input_layout_t {
public:
    input_layout_t() = default;

    auto element_vertex_add(const std::string& semantic, std::uint32_t semantic_index
    , DXGI_FORMAT format, UINT slot) -> void;
    auto element_instance_add(const std::string& semantic, std::uint32_t semantic_index
    , DXGI_FORMAT format, UINT slot, std::uint32_t instance_count) -> void;
    auto desc_get() const -> D3D12_INPUT_LAYOUT_DESC;
private:
    std::vector<std::unique_ptr<std::string>> m_semantic{};
    std::vector<D3D12_INPUT_ELEMENT_DESC> m_element{};
};

auto input_layout_t::element_vertex_add(const std::string& semantic, std::uint32_t semantic_index
, DXGI_FORMAT format, UINT slot) -> void {
    D3D12_INPUT_ELEMENT_DESC element{};
    m_semantic.push_back(std::make_unique<std::string>(semantic));
    element.SemanticName = m_semantic.back()->c_str();
    element.SemanticIndex = semantic_index;
    element.Format = format;
    element.InputSlot = slot;
    element.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    element.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    //The number of instances to draw using the same per-instance data before advancing in the buffer by one element.
    //This value must be 0 for an element that contains per-vertex data.
    element.InstanceDataStepRate = 0;
    m_element.push_back(element);
    return;
}

auto input_layout_t::element_instance_add(const std::string& semantic, std::uint32_t semantic_index
, DXGI_FORMAT format, UINT slot, std::uint32_t instance_count) -> void {
    D3D12_INPUT_ELEMENT_DESC element{};
    m_semantic.push_back(std::make_unique<std::string>(semantic));
    element.SemanticName = m_semantic.back()->c_str();
    element.SemanticIndex = semantic_index;
    element.Format = format;
    element.InputSlot = slot;
    element.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    element.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
    element.InstanceDataStepRate = instance_count;
    m_element.push_back(element);
    return;
}

auto input_layout_t::desc_get() const -> D3D12_INPUT_LAYOUT_DESC {
    D3D12_INPUT_LAYOUT_DESC desc{};
    desc.pInputElementDescs = m_element.data();
    desc.NumElements = static_cast<UINT>(m_element.size());
    return desc;
}

#endif