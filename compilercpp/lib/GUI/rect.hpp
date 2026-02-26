#ifndef COMPILERCPP_LIB_GUI_RECT
#define COMPILERCPP_LIB_GUI_RECT

#include <array>
#include <cstring>

#include "stu.hpp"

class rect_t {
private:
    struct vertex_data {
        DirectX::XMFLOAT4 pos{};
        DirectX::XMFLOAT4 color{};
    };

    static Microsoft::WRL::ComPtr<ID3D12Device2> m_device;
    static Microsoft::WRL::ComPtr<ID3D12RootSignature> m_root_signature;
    static Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipeline_state;
    static size_2D m_window_size;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertex_buffer{};
    D3D12_VERTEX_BUFFER_VIEW m_vertex_buffer_view{};
public:
    static auto init(Microsoft::WRL::ComPtr<ID3D12Device2> device, size_2D window_size) -> void {
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

    static auto render_begin(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list) -> void {
        command_list->SetGraphicsRootSignature(m_root_signature.Get());
        command_list->SetPipelineState(m_pipeline_state.Get());
        command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        return;
    }

    static auto render_end() -> void {
        return;
    }

    rect_t() {}

    rect_t(pos_2D pos, size_2D size, color_t color, float depth) {
        float pos_x{ static_cast<float>(pos.x) / static_cast<float>(m_window_size.x) * 2.0f - 1.0f };
        float pos_y{ 1.0f - static_cast<float>(pos.y) / static_cast<float>(m_window_size.y) * 2.0f };
        float size_x{ static_cast<float>(size.x) / static_cast<float>(m_window_size.x) * 2.0f };
        float size_y{ static_cast<float>(size.y) / static_cast<float>(m_window_size.y) * 2.0f };
        std::array vertex{
            vertex_data{ { pos_x, pos_y, depth, 1.0f }, { color.R, color.G, color.B, 1.0f } },
            vertex_data{ { pos_x + size_x, pos_y, depth, 1.0f }, { color.R, color.G, color.B, 1.0f } },
            vertex_data{ { pos_x, pos_y - size_y, depth, 1.0f }, { color.R, color.G, color.B, 1.0f } },
            vertex_data{ { pos_x + size_x, pos_y - size_y, depth, 1.0f }, { color.R, color.G, color.B, 1.0f } }
        };
        const UINT64 vertex_size{ vertex.size() * sizeof(vertex_data) };

        auto upload_heap_property{ create_upload_heap_property() };
        D3D12_RESOURCE_DESC upload_resource_description{};
        upload_resource_description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        upload_resource_description.Alignment = 0;
        upload_resource_description.Width = vertex_size;
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

        void* upload_resource_cpu_address{};
        m_vertex_buffer->Map(0, nullptr, &upload_resource_cpu_address);
        std::memcpy(upload_resource_cpu_address, vertex.data(), vertex_size);
        m_vertex_buffer->Unmap(0, nullptr);
        m_vertex_buffer_view.BufferLocation = m_vertex_buffer->GetGPUVirtualAddress();
        m_vertex_buffer_view.SizeInBytes = vertex_size;
        m_vertex_buffer_view.StrideInBytes = sizeof(vertex_data);

        return;
    }

    auto render(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list) -> void {
        command_list->IASetVertexBuffers(0, 1, &m_vertex_buffer_view);
        command_list->DrawInstanced(4, 1, 0, 0);
        return;
    }
};

Microsoft::WRL::ComPtr<ID3D12Device2> rect_t::m_device{};
Microsoft::WRL::ComPtr<ID3D12RootSignature> rect_t::m_root_signature{};
Microsoft::WRL::ComPtr<ID3D12PipelineState> rect_t::m_pipeline_state{};
size_2D rect_t::m_window_size{};

#endif