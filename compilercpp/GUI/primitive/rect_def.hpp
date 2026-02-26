#ifndef COMPILERCPP_GUI_PRIMITIVE_RECT_DEF
#define COMPILERCPP_GUI_PRIMITIVE_RECT_DEF

#include <array>
#include <cstring>
#include <string>
#include <vector>
#include "../../lib/header.hpp"

#include "engine_decl.hpp"

#include "../../lib/stu.hpp"
#include "../directx/create.hpp"

class rect_primitive_t {
private:
    struct vertex_data_t {
        DirectX::XMFLOAT4 pos{};
        DirectX::XMFLOAT4 color{};
    };

    static Microsoft::WRL::ComPtr<ID3D12Device2> m_device;
    static Microsoft::WRL::ComPtr<ID3D12RootSignature> m_root_signature;
    static Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipeline_state;
    static size_2D m_window_size;

    engine_t* m_engine{};
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertex_buffer{};
    D3D12_VERTEX_BUFFER_VIEW m_vertex_buffer_view{};
    pos_2D m_pos{};
    size_2D m_size{};
    float m_depth{};
    color_t m_color{};
    std::recursive_mutex m_mutex{};

    constexpr static std::size_t vertex_data_len{ 4 };
    constexpr static std::size_t vertex_data_size{ sizeof(vertex_data_t) * vertex_data_len };

    auto upload_vertex_data() -> void;
public:
    static auto init(engine_t* engine, Microsoft::WRL::ComPtr<ID3D12Device2> device, size_2D window_size) -> void;
    static auto uninit() -> void;
    static auto render_begin(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list, engine_t* engine) -> void;
    static auto render_end() -> void;

    rect_primitive_t() = default;
    rect_primitive_t(engine_t* engine, pos_2D pos, size_2D size, float depth, color_t color);
    
    auto set_pos(pos_2D pos) -> void;
    auto set_size(size_2D size) -> void;
    auto set_color(color_t color) -> void;
    auto render(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list) -> void;
    auto inside(pos_2D pos) -> bool;
};

Microsoft::WRL::ComPtr<ID3D12Device2> rect_primitive_t::m_device{};
Microsoft::WRL::ComPtr<ID3D12RootSignature> rect_primitive_t::m_root_signature{};
Microsoft::WRL::ComPtr<ID3D12PipelineState> rect_primitive_t::m_pipeline_state{};
size_2D rect_primitive_t::m_window_size{};

#endif