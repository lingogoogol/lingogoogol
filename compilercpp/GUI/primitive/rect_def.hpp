#ifndef COMPILERCPP_GUI_PRIMITIVE_RECT_DEF
#define COMPILERCPP_GUI_PRIMITIVE_RECT_DEF

#include <array>
#include <cstring>
#include <string>
#include <vector>
#include "../../lib/header.hpp"

#include "engine_decl.hpp"

#include "../../lib/stu.hpp"
#include "../directx/SRV_impl.hpp"
#include "../directx/create.hpp"
#include "../directx/root_signature.hpp"

class rect_primitive_t {
private:
    struct vertex_data_solid_t {
        DirectX::XMFLOAT4 pos{};
        DirectX::XMFLOAT4 color{};
    };

    struct vertex_data_texture_t {
        DirectX::XMFLOAT4 pos{};
        DirectX::XMFLOAT2 texcoord{};
        std::uint32_t texture_index{};
    };

    static Microsoft::WRL::ComPtr<ID3D12Device2> m_device;
    static root_signature_t m_solid_root_signature;
    static root_signature_t m_texture_root_signature;
    static pipeline_state_t m_solid_pipeline_state;
    static pipeline_state_t m_texture_pipeline_state;
    static size_2D m_window_size;

    engine_t* m_engine{};
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertex_buffer{};
    D3D12_VERTEX_BUFFER_VIEW m_vertex_buffer_view{};
    pos_2D m_pos{};
    size_2D m_size{};
    float m_depth{};
    color_t m_color{};
    const SRV_t* m_SRV{};
    pos_2D m_texture_pos{};
    size_2D m_texture_axis_x{};
    size_2D m_texture_axis_y{};
    std::recursive_mutex m_mutex{};
    bool m_texture_enable{};
    pos_2D m_clip_pos{};
    size_2D m_clip_size{};
    std::string m_name{};

    constexpr static std::size_t vertex_data_len{ 4 };
    constexpr static std::size_t vertex_data_solid_size{ sizeof(vertex_data_solid_t) * vertex_data_len };
    constexpr static std::size_t vertex_data_texture_size{ sizeof(vertex_data_texture_t) * vertex_data_len };

    auto upload_vertex_data() -> void;
public:
    static auto init(Microsoft::WRL::ComPtr<ID3D12Device2> device, size_2D window_size) -> void;
    static auto uninit() -> void;
    static auto render(const std::set<rect_primitive_t*>& rect
    , Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list) -> void;

    rect_primitive_t() = default;
    rect_primitive_t(engine_t* engine, pos_2D pos, size_2D size, float depth, std::string name);
    rect_primitive_t(engine_t* engine, pos_2D pos, size_2D size, float depth, pos_2D clip_pos, size_2D clip_size, std::string name);
    rect_primitive_t(engine_t* engine, pos_2D pos, size_2D size, float depth, color_t color, std::string name);
    rect_primitive_t(engine_t* engine, pos_2D pos, size_2D size, float depth, const SRV_t& SRV, std::string name);
    rect_primitive_t(engine_t* engine, pos_2D pos, size_2D size, float depth, pos_2D clip_pos, size_2D clip_size
    , const SRV_t& SRV, pos_2D texture_pos, size_2D texture_axis_x, size_2D texture_axis_y, std::string name);
    
    auto set_pos(pos_2D pos) -> void;
    auto set_size(size_2D size) -> void;
    auto set_color(color_t color) -> void;
    auto set_SRV(const SRV_t& SRV) -> void;
    auto color_enable() -> void;
    auto texture_enable() -> void;
    auto inside(pos_2D pos) -> bool;
private:
    auto render(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list) -> void;
};

#endif