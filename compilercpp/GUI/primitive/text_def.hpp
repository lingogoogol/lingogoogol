#ifndef COMPILERCPP_GUI_PRIMITIVE_TEXT_DEF
#define COMPILERCPP_GUI_PRIMITIVE_TEXT_DEF

#include <vector>
#include <string>
#include <array>
#include <mutex>

#include "engine_decl.hpp"

#include "../../lib/header.hpp"
#include "../../lib/stu.hpp"

class text_primitive_t {
private:
    static Microsoft::WRL::ComPtr<IDWriteFactory5> m_factory_DWrite;
    static Microsoft::WRL::ComPtr<IDWriteInMemoryFontFileLoader> m_font_file_loader;
    static Microsoft::WRL::ComPtr<IDWriteFontCollection1> m_font_collection;
    static Microsoft::WRL::ComPtr<ID3D11On12Device> m_device_D3D11on12;
    static Microsoft::WRL::ComPtr<ID2D1DeviceContext> m_device_context_D2D1;
    static Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_device_context_D3D11;
    static std::vector<Microsoft::WRL::ComPtr<ID3D11Resource>> m_RT_D3D11;
    static std::vector<Microsoft::WRL::ComPtr<ID2D1Bitmap1>> m_RT_D2D1;

    engine_t* m_engine{};
    std::wstring m_text{};
    pos_2D m_text_pos{};
    size_2D m_text_size{};
    pos_2D m_clip_pos{};
    size_2D m_clip_size{};
    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_format{};
    Microsoft::WRL::ComPtr<IDWriteTextLayout> m_layout{};
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_brush{};
    std::recursive_mutex m_mutex{};

    auto create_layout() -> void;
    auto create_brush(color_t color) -> void;
public:
    static auto init(Microsoft::WRL::ComPtr<ID3D12Device2> device_D3D12, Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue
    , HWND window, const std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>& RT) -> void;
    static auto uninit() -> void;
    static auto render_begin(std::size_t frame_index) -> void;
    static auto render_end(std::size_t frame_index) -> void;

    text_primitive_t() = default;
    text_primitive_t(engine_t* engine, std::wstring content, pos_2D text_pos, size_2D text_size
    , pos_2D clip_pos, size_2D clip_size, size_1D size_font, color_t color, alignment_2D alignment, bool wrap);
    
    auto get_text() -> std::wstring;
    auto set_text(std::wstring text) -> void;
    auto get_text_pos() -> pos_2D;
    auto set_text_pos(pos_2D text_pos) -> void;
    auto get_text_size() -> size_2D;
    auto set_text_size(size_2D text_size) -> void;
    auto get_clip_pos() -> pos_2D;
    auto set_clip_pos(pos_2D clip_pos) -> void;
    auto get_clip_size() -> size_2D;
    auto set_clip_size(size_2D clip_size) -> void;
    auto set_color(color_t color) -> void;
    auto get_render_size() -> size_2D;
    auto render() -> void;
};

Microsoft::WRL::ComPtr<IDWriteFactory5> text_primitive_t::m_factory_DWrite{};
Microsoft::WRL::ComPtr<IDWriteInMemoryFontFileLoader> text_primitive_t::m_font_file_loader{};
Microsoft::WRL::ComPtr<IDWriteFontCollection1> text_primitive_t::m_font_collection{};
Microsoft::WRL::ComPtr<ID3D11On12Device> text_primitive_t::m_device_D3D11on12{};
Microsoft::WRL::ComPtr<ID2D1DeviceContext> text_primitive_t::m_device_context_D2D1{};
Microsoft::WRL::ComPtr<ID3D11DeviceContext> text_primitive_t::m_device_context_D3D11{};
std::vector<Microsoft::WRL::ComPtr<ID3D11Resource>> text_primitive_t::m_RT_D3D11{};
std::vector<Microsoft::WRL::ComPtr<ID2D1Bitmap1>> text_primitive_t::m_RT_D2D1{};

#endif