#ifndef COMPILERCPP_GUI_PRIMITIVE_RECT_IMPL
#define COMPILERCPP_GUI_PRIMITIVE_RECT_IMPL

#include "rect_def.hpp"
#include "engine_def.hpp"

#include "../../lib/common.hpp"

Microsoft::WRL::ComPtr<ID3D12Device2> rect_primitive_t::m_device{};
root_signature_t rect_primitive_t::m_solid_root_signature{};
root_signature_t rect_primitive_t::m_texture_root_signature{};
pipeline_state_t rect_primitive_t::m_solid_pipeline_state{};
pipeline_state_t rect_primitive_t::m_texture_pipeline_state{};
size_2D rect_primitive_t::m_window_size{};

auto rect_primitive_t::upload_vertex_data() -> void {
    std::unique_lock lock{ m_mutex };
    float pos_x{ static_cast<float>(m_pos.x) / static_cast<float>(m_window_size.x) * 2.0f - 1.0f };
    float pos_y{ 1.0f - static_cast<float>(m_pos.y) / static_cast<float>(m_window_size.y) * 2.0f };
    float size_x{ static_cast<float>(m_size.x) / static_cast<float>(m_window_size.x) * 2.0f };
    float size_y{ -static_cast<float>(m_size.y) / static_cast<float>(m_window_size.y) * 2.0f };

    void* upload_resource_cpu_address{};
    hresult(m_vertex_buffer->Map(0, nullptr, &upload_resource_cpu_address));
    if (m_texture_enable) {
        auto texture_index{ static_cast<std::uint32_t>(m_SRV->descriptor_heap_index_get()) };
        float texture_pos_x{ static_cast<float>(m_pos.x + m_texture_pos.x) / static_cast<float>(m_window_size.x) * 2.0f - 1.0f };
        float texture_pos_y{ 1.0f - static_cast<float>(m_pos.y + m_texture_pos.y) / static_cast<float>(m_window_size.y) * 2.0f };
        float texture_axis_x_x{ static_cast<float>(m_texture_axis_x.x) / static_cast<float>(m_window_size.x) * 2.0f };
        float texture_axis_x_y{ -static_cast<float>(m_texture_axis_x.y) / static_cast<float>(m_window_size.y) * 2.0f };
        float texture_axis_y_x{ static_cast<float>(m_texture_axis_y.x) / static_cast<float>(m_window_size.x) * 2.0f };
        float texture_axis_y_y{ -static_cast<float>(m_texture_axis_y.y) / static_cast<float>(m_window_size.y) * 2.0f };
        std::array vertex_data{
            vertex_data_texture_t{ { texture_pos_x, texture_pos_y, m_depth, 1.0f }, { 0.0f, 0.0f }, texture_index },
            vertex_data_texture_t{ { texture_pos_x + texture_axis_x_x, texture_pos_y + texture_axis_x_y, m_depth, 1.0f }, { 1.0f, 0.0f }, texture_index },
            vertex_data_texture_t{ { texture_pos_x + texture_axis_y_x, texture_pos_y + texture_axis_y_y, m_depth, 1.0f }, { 0.0f, 1.0f }, texture_index },
            vertex_data_texture_t{ { texture_pos_x + texture_axis_x_x + texture_axis_y_x, texture_pos_y + texture_axis_x_y + texture_axis_y_y, m_depth, 1.0f }, { 1.0f, 1.0f }, texture_index }
        };
        std::memcpy(upload_resource_cpu_address, vertex_data.data(), vertex_data_texture_size);
        m_vertex_buffer_view.SizeInBytes = vertex_data_texture_size;
        m_vertex_buffer_view.StrideInBytes = sizeof(vertex_data_texture_t);
    }
    else {
        std::array vertex_data{
            vertex_data_solid_t{ { pos_x, pos_y, m_depth, 1.0f }, { m_color.R, m_color.G, m_color.B, 1.0f } },
            vertex_data_solid_t{ { pos_x + size_x, pos_y, m_depth, 1.0f }, { m_color.R, m_color.G, m_color.B, 1.0f } },
            vertex_data_solid_t{ { pos_x, pos_y + size_y, m_depth, 1.0f }, { m_color.R, m_color.G, m_color.B, 1.0f } },
            vertex_data_solid_t{ { pos_x + size_x, pos_y + size_y, m_depth, 1.0f }, { m_color.R, m_color.G, m_color.B, 1.0f } }
        };
        std::memcpy(upload_resource_cpu_address, vertex_data.data(), vertex_data_solid_size);
        m_vertex_buffer_view.SizeInBytes = vertex_data_solid_size;
        m_vertex_buffer_view.StrideInBytes = sizeof(vertex_data_solid_t);
    }
    m_vertex_buffer->Unmap(0, nullptr);
    m_vertex_buffer_view.BufferLocation = m_vertex_buffer->GetGPUVirtualAddress();
    m_engine->log_info_queue();
    return;
}

auto rect_primitive_t::init(Microsoft::WRL::ComPtr<ID3D12Device2> device, size_2D window_size) -> void {
    m_device = device;
    m_window_size = window_size;

    std::array<std::string, 2> shader_code{};
    m_solid_root_signature.flag_set(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    m_solid_root_signature.serialize(m_device);
    input_layout_t solid_input_layout{};
    solid_input_layout.element_vertex_add("POS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0);
    solid_input_layout.element_vertex_add("COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0);
    m_solid_pipeline_state.init(m_solid_root_signature, solid_input_layout
    , DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, "rect_primitive_t::m_solid_pipeline_state");
    shader_bytecode_t solid_vertex_shader{ 256, 1, "" };
    m_solid_pipeline_state.vertex_shader_set(solid_vertex_shader);
    shader_bytecode_t solid_pixel_shader{ 256, 2, "" };
    m_solid_pipeline_state.pixel_shader_set(solid_pixel_shader);
    m_solid_pipeline_state.create(m_device);

    m_texture_root_signature.flag_set(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    m_texture_root_signature.static_sampler_add(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    descriptor_table_t descriptor_table{};
    descriptor_table.range_add(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 0, 0, 0x1000);
    m_texture_root_signature.descriptor_table_add(descriptor_table, D3D12_SHADER_VISIBILITY_PIXEL);
    m_texture_root_signature.serialize(m_device);
    input_layout_t texture_input_layout{};
    texture_input_layout.element_vertex_add("POS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0);
    texture_input_layout.element_vertex_add("Texcoord", 0, DXGI_FORMAT_R32G32_FLOAT, 0);
    texture_input_layout.element_vertex_add("s_texture_index", 0, DXGI_FORMAT_R32_UINT, 0);
    m_texture_pipeline_state.init(m_texture_root_signature, texture_input_layout
    , DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, "rect_primitive_t::m_texture_pipeline_state");
    shader_bytecode_t texture_vertex_shader{ 256, 3, "" };
    m_texture_pipeline_state.vertex_shader_set(texture_vertex_shader);
    shader_bytecode_t texture_pixel_shader{ 256, 4, "" };
    m_texture_pipeline_state.pixel_shader_set(texture_pixel_shader);
    m_texture_pipeline_state.create(m_device);
    return;
}

auto rect_primitive_t::uninit() -> void {}

auto rect_primitive_t::render(const std::set<rect_primitive_t*>& rect
, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list) -> void {
    command_list->SetGraphicsRootSignature(m_solid_root_signature.interface_get().Get());
    command_list->SetPipelineState(m_solid_pipeline_state.interface_get().Get());
    command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    for (auto i{ rect.begin() }; i != rect.end(); ++i) {
        if (!(*i)->m_texture_enable) {
            (*i)->render(command_list);
        }
    }
    
    command_list->SetGraphicsRootSignature(m_texture_root_signature.interface_get().Get());
    command_list->SetPipelineState(m_texture_pipeline_state.interface_get().Get());
    command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    for (auto i{ rect.begin() }; i != rect.end(); ++i) {
        if ((*i)->m_texture_enable) {
            (*i)->render(command_list);
        }
    }
    return;
}

rect_primitive_t::rect_primitive_t(engine_t* engine, pos_2D pos, size_2D size, float depth, std::string name)
: rect_primitive_t{ engine, pos, size, depth, pos, size, name } {
    m_clip_follows_size = true;
}

rect_primitive_t::rect_primitive_t(engine_t* engine, pos_2D pos, size_2D size, float depth, pos_2D clip_pos, size_2D clip_size, std::string name)
: m_engine{ engine }, m_pos{ pos }, m_size{ size }, m_depth{ depth }, m_clip_pos{ clip_pos }, m_clip_size{ clip_size }, m_name{ name } {}

rect_primitive_t::rect_primitive_t(engine_t* engine, pos_2D pos, size_2D size, float depth, color_t color, std::string name)
: rect_primitive_t{ engine, pos, size, depth, name } {
    m_texture_enable = false;
    m_color = color;
    m_vertex_buffer = create_resource_upload(m_device, std::max(vertex_data_solid_size, vertex_data_texture_size), name + ".m_vertex_buffer");
    upload_vertex_data();
    return;
}

rect_primitive_t::rect_primitive_t(engine_t* engine, pos_2D pos, size_2D size, float depth, const SRV_t& SRV, std::string name)
: rect_primitive_t{ engine, pos, size, depth, pos, size, SRV, pos_2D{ 0, 0 }, size_2D{ size.x, 0 }, size_2D{ 0, size.y }, name } {
    m_clip_follows_size = true;
}

rect_primitive_t::rect_primitive_t(engine_t* engine, pos_2D pos, size_2D size, float depth, pos_2D clip_pos, size_2D clip_size
, const SRV_t& SRV, pos_2D texture_pos, size_2D texture_axis_x, size_2D texture_axis_y, std::string name)
: rect_primitive_t{ engine, pos, size, depth, clip_pos, clip_size, name } {
    m_texture_enable = true;
    m_SRV = &SRV;
    m_texture_pos = texture_pos;
    m_texture_axis_x = texture_axis_x;
    m_texture_axis_y = texture_axis_y;
    m_vertex_buffer = create_resource_upload(m_device, std::max(vertex_data_solid_size, vertex_data_texture_size), name + ".m_vertex_buffer");
    upload_vertex_data();
    return;
}

auto rect_primitive_t::set_pos(pos_2D pos) -> void {
    std::unique_lock lock{ m_mutex };
    m_clip_pos = m_clip_pos + (pos - m_pos);
    m_pos = pos;
    m_engine->flush();
    upload_vertex_data();
    m_engine->redraw();
    return;
}

auto rect_primitive_t::set_size(size_2D size) -> void {
    std::unique_lock lock{ m_mutex };
    m_size = size;
    if (m_clip_follows_size) {
        m_clip_size = size;
    }
    m_engine->flush();
    upload_vertex_data();
    m_engine->redraw();
    return;
}

auto rect_primitive_t::set_color(color_t color) -> void {
    std::unique_lock lock{ m_mutex };
    m_color = color;
    m_engine->flush();
    upload_vertex_data();
    m_engine->redraw();
    return;
}

auto rect_primitive_t::set_SRV(const SRV_t& SRV) -> void {
    std::unique_lock lock{ m_mutex };
    m_SRV = &SRV;
    m_engine->flush();
    upload_vertex_data();
    m_engine->redraw();
    return;
}

auto rect_primitive_t::color_enable() -> void {
    std::unique_lock lock{ m_mutex };
    m_texture_enable = false;
    return;
}

auto rect_primitive_t::texture_enable() -> void {
    std::unique_lock lock{ m_mutex };
    m_texture_enable = true;
    return;
}

auto rect_primitive_t::inside(pos_2D pos) -> bool {
    std::unique_lock lock{ m_mutex };
    return ::inside(m_pos, m_size, pos);
}

auto rect_primitive_t::render(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list) -> void {
    std::unique_lock lock{ m_mutex };
    //(0, 0) is at the top left corner of the window.
    D3D12_RECT scissor_rect{};
    scissor_rect.left = static_cast<LONG>(m_clip_pos.x);
    scissor_rect.right = static_cast<LONG>(m_clip_pos.x + m_clip_size.x);
    scissor_rect.top = static_cast<LONG>(m_clip_pos.y);
    scissor_rect.bottom = static_cast<LONG>(m_clip_pos.y + m_clip_size.y);
    if (m_texture_enable) {
        ID3D12DescriptorHeap* descriptor_heap{ m_SRV->descriptor_heap_get().interface_get().Get() };
        command_list->SetDescriptorHeaps(1, &descriptor_heap);
        command_list->SetGraphicsRootDescriptorTable(0, m_SRV->descriptor_heap_get().descriptor_handle_GPU_get(0));
    }
    command_list->RSSetScissorRects(1, &scissor_rect);
    command_list->IASetVertexBuffers(0, 1, &m_vertex_buffer_view);
    command_list->DrawInstanced(4, 1, 0, 0);
    m_engine->log_info_queue();
    return;
}

#endif
