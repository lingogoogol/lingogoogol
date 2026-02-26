#ifndef COMPILERCPP_GUI_DIRECTX_PIPELINE_STATE
#define COMPILERCPP_GUI_DIRECTX_PIPELINE_STATE

#include "root_signature.hpp"
#include "input_layout.hpp"
#include "shader_bytecode.hpp"

class pipeline_state_t {
public:
    pipeline_state_t() = default;
    pipeline_state_t(const root_signature_t& root_signature, const input_layout_t& input_layout
    , DXGI_FORMAT render_target_format, DXGI_FORMAT depth_stencil_format);

    auto init(const root_signature_t& root_signature, const input_layout_t& input_layout
    , DXGI_FORMAT render_target_format, DXGI_FORMAT depth_stencil_format) -> void;
    auto vertex_shader_set(const shader_bytecode_t& bytecode) -> void;
    auto pixel_shader_set(const shader_bytecode_t& bytecode) -> void;
    auto domain_shader_set(const shader_bytecode_t& bytecode) -> void;
    auto hull_shader_set(const shader_bytecode_t& bytecode) -> void;
    auto geometry_shader_set(const shader_bytecode_t& bytecode) -> void;
    auto create(Microsoft::WRL::ComPtr<ID3D12Device> device) -> void;
    auto interface_get() const -> Microsoft::WRL::ComPtr<ID3D12PipelineState>;
private:
    D3D12_GRAPHICS_PIPELINE_STATE_DESC m_desc{};
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_interface{};
};

pipeline_state_t::pipeline_state_t(const root_signature_t& root_signature, const input_layout_t& input_layout
, DXGI_FORMAT render_target_format, DXGI_FORMAT depth_stencil_format) {
    init(root_signature, input_layout, render_target_format, depth_stencil_format);
    return;
}

auto pipeline_state_t::init(const root_signature_t& root_signature, const input_layout_t& input_layout
, DXGI_FORMAT render_target_format, DXGI_FORMAT depth_stencil_format) -> void {
    m_desc.pRootSignature = root_signature.interface_get().Get();
    m_desc.BlendState.AlphaToCoverageEnable = false;
    m_desc.BlendState.IndependentBlendEnable = false;
    m_desc.BlendState.RenderTarget[0].BlendEnable = true;
    m_desc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    m_desc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    m_desc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    m_desc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO;
    m_desc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
    m_desc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    //BlendEnable and LogicOpEnable cannot be true simultaneously.
    m_desc.BlendState.RenderTarget[0].LogicOpEnable = false;
    m_desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    m_desc.SampleMask = static_cast<UINT>(-1);
    m_desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    //Do not draw triangles that are back-facing.
    //DirectX coordinate system:
    //       +1.0f
    //         ^
    //         |
    //-1.0f ---+--> +1.0f
    //         |
    //         |
    //       -1.0f
    m_desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    m_desc.RasterizerState.FrontCounterClockwise = true;
    m_desc.RasterizerState.DepthBias = 0;
    m_desc.RasterizerState.SlopeScaledDepthBias = 0.0f;
    m_desc.RasterizerState.DepthBiasClamp = 0.0f;
    m_desc.RasterizerState.DepthClipEnable = true;
    m_desc.RasterizerState.MultisampleEnable = false;
    m_desc.RasterizerState.AntialiasedLineEnable = false;
    m_desc.RasterizerState.ForcedSampleCount = 0;
    m_desc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    m_desc.DepthStencilState.DepthEnable = true;
    //Turn on writes to the depth-stencil buffer.
    m_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    m_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    m_desc.DepthStencilState.StencilEnable = false;
    m_desc.InputLayout = input_layout.desc_get();
    m_desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    m_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    m_desc.NumRenderTargets = 1;
    m_desc.RTVFormats[0] = render_target_format;
    m_desc.DSVFormat = depth_stencil_format;
    m_desc.SampleDesc.Count = 1;
    m_desc.SampleDesc.Quality = 0;
    m_desc.NodeMask = 0;
    m_desc.CachedPSO.pCachedBlob = nullptr;
    m_desc.CachedPSO.CachedBlobSizeInBytes = 0;
    m_desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    return;
}

auto pipeline_state_t::vertex_shader_set(const shader_bytecode_t& bytecode) -> void {
    m_desc.VS = bytecode.desc_get();
    return;
}

auto pipeline_state_t::pixel_shader_set(const shader_bytecode_t& bytecode) -> void {
    m_desc.PS = bytecode.desc_get();
    return;
}

auto pipeline_state_t::domain_shader_set(const shader_bytecode_t& bytecode) -> void {
    m_desc.DS = bytecode.desc_get();
    return;
}

auto pipeline_state_t::hull_shader_set(const shader_bytecode_t& bytecode) -> void {
    m_desc.HS = bytecode.desc_get();
    return;
}

auto pipeline_state_t::geometry_shader_set(const shader_bytecode_t& bytecode) -> void {
    m_desc.GS = bytecode.desc_get();
    return;
}

auto pipeline_state_t::create(Microsoft::WRL::ComPtr<ID3D12Device> device) -> void {
    hresult(device->CreateGraphicsPipelineState(&m_desc, IID_PPV_ARGS(&m_interface)));
    return;
}

auto pipeline_state_t::interface_get() const -> Microsoft::WRL::ComPtr<ID3D12PipelineState> {
    return m_interface;
}

#endif