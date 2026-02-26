#ifndef COMPILERCPP_GUI_DIRECTX_TEXTURE
#define COMPILERCPP_GUI_DIRECTX_TEXTURE

#include "pixmap.hpp"
#include "create.hpp"

class texture_t {
public:
    texture_t() = default;
    texture_t(Microsoft::WRL::ComPtr<ID3D12Device> device
    , Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list, const pixmap_t& pixmap, std::string name);
    auto init(Microsoft::WRL::ComPtr<ID3D12Device> device
    , Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list, const pixmap_t& pixmap, std::string name) -> void;

    auto format_get() const -> DXGI_FORMAT;
    auto miplevel_count_get() const -> std::uint16_t;
    auto interface_get() const -> Microsoft::WRL::ComPtr<ID3D12Resource>;
private:
    //Create m_resource without changing m_pixmap.
    auto init1(Microsoft::WRL::ComPtr<ID3D12Device> device
    , Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list) -> void;
    
    const pixmap_t* m_pixmap{};
    Microsoft::WRL::ComPtr<ID3D12Resource> m_resource{};
    Microsoft::WRL::ComPtr<ID3D12Resource> m_upload_resource{};
    DXGI_FORMAT m_format{};
    std::uint16_t m_miplevel_count{};
    std::string m_name{};

    friend class descriptor_heap_t;
};

texture_t::texture_t(Microsoft::WRL::ComPtr<ID3D12Device> device
, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list
, const pixmap_t& pixmap, std::string name): m_pixmap{ &pixmap }, m_name{ name } {
    init1(device, command_list);
    return;
}

auto texture_t::init(Microsoft::WRL::ComPtr<ID3D12Device> device
, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list, const pixmap_t& pixmap, std::string name) -> void {
    m_pixmap = &pixmap;
    m_name = name;
    init1(device, command_list);
    return;
}

auto texture_t::format_get() const -> DXGI_FORMAT {
    return m_format;
}

auto texture_t::miplevel_count_get() const -> std::uint16_t {
    return m_miplevel_count;
}

auto texture_t::interface_get() const -> Microsoft::WRL::ComPtr<ID3D12Resource> {
    return m_resource;
}

auto texture_t::init1(Microsoft::WRL::ComPtr<ID3D12Device> device
, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list) -> void {
    m_format = DXGI_FORMAT_R8_UNORM;
    m_miplevel_count = 1;
    
    D3D12_RESOURCE_DESC resource_desc{};
    resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resource_desc.Alignment = 0;
    resource_desc.Width = static_cast<UINT>(m_pixmap->size_get().x);
    resource_desc.Height = static_cast<UINT>(m_pixmap->size_get().y);
    resource_desc.DepthOrArraySize = 1;
    resource_desc.MipLevels = m_miplevel_count;
    resource_desc.Format = m_format;
    resource_desc.SampleDesc.Count = 1;
    resource_desc.SampleDesc.Quality = 0;
    resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
    std::uint64_t upload_size{};
    device->GetCopyableFootprints(&resource_desc, 0, 1, 0, &footprint, nullptr, nullptr, &upload_size);
    m_upload_resource = create_resource_upload(device, upload_size, m_name + ".m_upload_resource");
    std::byte* dest_ptr{};
    hresult(m_upload_resource->Map(0, nullptr, reinterpret_cast<void**>(&dest_ptr)));
    for (std::uint64_t y{ 0 }; y < static_cast<std::uint64_t>(m_pixmap->size_get().y); ++y) {
        for (std::uint64_t x{ 0 }; x < static_cast<std::uint64_t>(m_pixmap->size_get().x); ++x) {
            std::uint64_t offset{ footprint.Footprint.RowPitch * y + x };
            dest_ptr[offset] = (*m_pixmap)[x, y];
        }
    }
    m_upload_resource->Unmap(0, nullptr);

    D3D12_HEAP_PROPERTIES default_heap_properties{ create_default_heap_property() };
    hresult(device->CreateCommittedResource(&default_heap_properties, D3D12_HEAP_FLAG_NONE
    , &resource_desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_resource)));
    D3D12_set_name(m_resource, m_name + ".m_resource");
    D3D12_TEXTURE_COPY_LOCATION src_location{ create_copy_location(m_upload_resource, footprint) };
    D3D12_TEXTURE_COPY_LOCATION dest_location{ create_copy_location(m_resource, 0) };
    command_list->CopyTextureRegion(&dest_location, 0, 0, 0, &src_location, nullptr);
    D3D12_RESOURCE_BARRIER resource_barrier{};
    resource_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    resource_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    resource_barrier.Transition.pResource = m_resource.Get();
    resource_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    resource_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    resource_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    command_list->ResourceBarrier(1, &resource_barrier);
    return;
}

#endif