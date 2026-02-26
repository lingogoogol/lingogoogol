#ifndef COMPILERCPP_LIB_GUI_PRIMITIVE_RECT_IMPL
#define COMPILERCPP_LIB_GUI_PRIMITIVE_RECT_IMPL

#include "rect_def.hpp"
#include "engine_def.hpp"

#include "../common.hpp"

auto rect_primitive_t::upload_vertex_data() -> void {
    float pos_x{ static_cast<float>(m_pos.x) / static_cast<float>(m_window_size.x) * 2.0f - 1.0f };
    float pos_y{ 1.0f - static_cast<float>(m_pos.y) / static_cast<float>(m_window_size.y) * 2.0f };
    float size_x{ static_cast<float>(m_size.x) / static_cast<float>(m_window_size.x) * 2.0f };
    float size_y{ static_cast<float>(m_size.y) / static_cast<float>(m_window_size.y) * 2.0f };
    std::array vertex_data{
        vertex_data_t{ { pos_x, pos_y, m_depth, 1.0f }, { m_color.R, m_color.G, m_color.B, 1.0f } },
        vertex_data_t{ { pos_x + size_x, pos_y, m_depth, 1.0f }, { m_color.R, m_color.G, m_color.B, 1.0f } },
        vertex_data_t{ { pos_x, pos_y - size_y, m_depth, 1.0f }, { m_color.R, m_color.G, m_color.B, 1.0f } },
        vertex_data_t{ { pos_x + size_x, pos_y - size_y, m_depth, 1.0f }, { m_color.R, m_color.G, m_color.B, 1.0f } }
    };

    void* upload_resource_cpu_address{};
    m_vertex_buffer->Map(0, nullptr, &upload_resource_cpu_address);
    std::memcpy(upload_resource_cpu_address, vertex_data.data(), vertex_data_size);
    m_vertex_buffer->Unmap(0, nullptr);
    m_vertex_buffer_view.BufferLocation = m_vertex_buffer->GetGPUVirtualAddress();
    m_vertex_buffer_view.SizeInBytes = vertex_data_size;
    m_vertex_buffer_view.StrideInBytes = sizeof(vertex_data_t);
    return;
}

auto rect_primitive_t::init(Microsoft::WRL::ComPtr<ID3D12Device2> device, size_2D window_size) -> void {
    m_device = device;
    m_window_size = window_size;

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_description{};
    root_signature_description.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    root_signature_description.Desc_1_1.NumParameters = 0;
    root_signature_description.Desc_1_1.pParameters = nullptr;
    root_signature_description.Desc_1_1.NumStaticSamplers = 0;
    root_signature_description.Desc_1_1.pStaticSamplers = nullptr;
    root_signature_description.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    Microsoft::WRL::ComPtr<ID3DBlob> root_signature_blob{};
    Microsoft::WRL::ComPtr<ID3DBlob> error_blob{};
    D3D12SerializeVersionedRootSignature(&root_signature_description, &root_signature_blob, &error_blob);
    m_device->CreateRootSignature(0, root_signature_blob->GetBufferPointer(), root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&m_root_signature));

    std::array<std::string, 2> shader_code{};
    std::vector<D3D12_INPUT_ELEMENT_DESC> input_element{};
    std::array<std::string, 2> input_element_name{ "POS", "COLOR" };
    input_element.push_back(create_input_element(input_element_name[0], DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0));
    input_element.push_back(create_input_element(input_element_name[1], DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0));
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_state_description{};
    pipeline_state_description.pRootSignature = m_root_signature.Get();
    pipeline_state_description.VS = create_shader_bytecode("vertex.cso", shader_code[0]);
    pipeline_state_description.PS = create_shader_bytecode("pixel.cso", shader_code[1]);
    pipeline_state_description.BlendState.AlphaToCoverageEnable = false;
    pipeline_state_description.BlendState.IndependentBlendEnable = false;
    pipeline_state_description.BlendState.RenderTarget[0].BlendEnable = false;
    pipeline_state_description.BlendState.RenderTarget[0].LogicOpEnable = false;
    pipeline_state_description.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    pipeline_state_description.SampleMask = -1;
    pipeline_state_description.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    pipeline_state_description.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    pipeline_state_description.RasterizerState.FrontCounterClockwise = false;
    pipeline_state_description.RasterizerState.DepthClipEnable = true;
    pipeline_state_description.RasterizerState.DepthClipEnable = false;
    pipeline_state_description.RasterizerState.AntialiasedLineEnable = false;
    pipeline_state_description.RasterizerState.ForcedSampleCount = 0;
    pipeline_state_description.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    pipeline_state_description.DepthStencilState.DepthEnable = true;
    pipeline_state_description.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    pipeline_state_description.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    pipeline_state_description.DepthStencilState.StencilEnable = false;
    pipeline_state_description.InputLayout.pInputElementDescs = input_element.data();
    pipeline_state_description.InputLayout.NumElements = input_element.size();
    pipeline_state_description.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipeline_state_description.NumRenderTargets = 1;
    pipeline_state_description.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pipeline_state_description.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    pipeline_state_description.SampleDesc.Count = 1;
    pipeline_state_description.SampleDesc.Quality = 0;
    pipeline_state_description.NodeMask = 0;
    pipeline_state_description.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    m_device->CreateGraphicsPipelineState(&pipeline_state_description, IID_PPV_ARGS(&m_pipeline_state));

    return;
}

auto rect_primitive_t::render_begin(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list) -> void {
    command_list->SetGraphicsRootSignature(m_root_signature.Get());
    command_list->SetPipelineState(m_pipeline_state.Get());
    command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    return;
}

auto rect_primitive_t::render_end() -> void {
    return;
}

rect_primitive_t::rect_primitive_t(engine_t* engine, pos_2D pos, size_2D size, float depth, color_t color)
: m_engine{ engine }, m_pos{ pos }, m_size{ size }, m_depth{ depth }, m_color{ color } {
    auto upload_heap_property{ create_upload_heap_property() };
    D3D12_RESOURCE_DESC upload_resource_description{};
    upload_resource_description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    upload_resource_description.Alignment = 0;
    upload_resource_description.Width = vertex_data_size;
    upload_resource_description.Height = 1;
    upload_resource_description.DepthOrArraySize = 1;
    upload_resource_description.MipLevels = 1;
    upload_resource_description.Format = DXGI_FORMAT_UNKNOWN;
    upload_resource_description.SampleDesc.Count = 1;
    upload_resource_description.SampleDesc.Quality = 0;
    upload_resource_description.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    upload_resource_description.Flags = D3D12_RESOURCE_FLAG_NONE;
    m_device->CreateCommittedResource(&upload_heap_property, D3D12_HEAP_FLAG_NONE, &upload_resource_description
    , D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_vertex_buffer));
    upload_vertex_data();
    return;
}

auto rect_primitive_t::set_color(color_t color) -> void {
    m_color = color;
    m_engine->flush();
    upload_vertex_data();
    m_engine->redraw();
    return;
}

auto rect_primitive_t::render(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list) -> void {
    command_list->IASetVertexBuffers(0, 1, &m_vertex_buffer_view);
    command_list->DrawInstanced(4, 1, 0, 0);
    return;
}

auto rect_primitive_t::inside(pos_2D pos) -> bool {
    return ::inside(m_pos, m_size, pos);
}

#endif