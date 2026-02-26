#ifndef COMPILERCPP_LIB_GUI_PRIMITIVE_TEXT_IMPL
#define COMPILERCPP_LIB_GUI_PRIMITIVE_TEXT_IMPL

#include "text_def.hpp"
#include "engine_def.hpp"

auto text_primitive_t::create_brush(color_t color) -> void {
    m_device_context_D2D1->CreateSolidColorBrush({ color.R, color.G, color.B, 1.0f }, &m_brush);
    return;
}

auto text_primitive_t::init(Microsoft::WRL::ComPtr<ID3D12Device2> device_D3D12, Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue
, HWND window, const std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>& RT) -> void {
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory5), &m_factory_DWrite);
    Microsoft::WRL::ComPtr<ID3D11Device> device_D3D11{};
    D3D11On12CreateDevice(device_D3D12.Get(), D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, reinterpret_cast<IUnknown**>(command_queue.GetAddressOf())
    , 1, 0, &device_D3D11, &m_device_context_D3D11, nullptr);
    device_D3D11.As(&m_device_D3D11on12);
    Microsoft::WRL::ComPtr<ID2D1Factory1> factory_D2D1{};
    D2D1_FACTORY_OPTIONS factory_D2D1_opt{};
    factory_D2D1_opt.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
    D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, __uuidof(ID2D1Factory1), &factory_D2D1_opt, &factory_D2D1);
    Microsoft::WRL::ComPtr<IDXGIDevice> device_DXGI{};
    m_device_D3D11on12.As(&device_DXGI);
    Microsoft::WRL::ComPtr<ID2D1Device> device_D2D1{};
    factory_D2D1->CreateDevice(device_DXGI.Get(), &device_D2D1);
    device_D2D1->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_device_context_D2D1);

    Microsoft::WRL::ComPtr<IDWriteFontSetBuilder1> font_set_builder{};
    m_factory_DWrite->CreateFontSetBuilder(&font_set_builder);
    Microsoft::WRL::ComPtr<IDWriteFontFile> font_file{};
    m_factory_DWrite->CreateFontFileReference(L"NotoSansTC-VariableFont_wght.ttf", nullptr, &font_file);
    font_set_builder->AddFontFile(font_file.Get());
    Microsoft::WRL::ComPtr<IDWriteFontSet> font_set{};
    font_set_builder->CreateFontSet(&font_set);
    Microsoft::WRL::ComPtr<IDWriteFontCollection1> font_collection{};
    m_factory_DWrite->CreateFontCollectionFromFontSet(font_set.Get(), &font_collection);
    
    m_RT_D3D11.resize(RT.size());
    for (std::size_t i{ 0 }; i < RT.size(); ++i) {
        D3D11_RESOURCE_FLAGS resource_flag{};
        resource_flag.BindFlags = D3D11_BIND_RENDER_TARGET;
        resource_flag.MiscFlags = 0;
        m_device_D3D11on12->CreateWrappedResource(RT[i].Get(), &resource_flag
        , D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT, IID_PPV_ARGS(&m_RT_D3D11[i]));
    }
    m_RT_D2D1.resize(m_RT_D3D11.size());
    for (std::size_t i{ 0 }; i < m_RT_D3D11.size(); ++i) {
        Microsoft::WRL::ComPtr<IDXGISurface> surface{};
        m_RT_D3D11[i].As(&surface);
        UINT DPI{ GetDpiForWindow(window) };
        D2D1_BITMAP_PROPERTIES1 bitmap_property{};
        bitmap_property.pixelFormat.format = DXGI_FORMAT_UNKNOWN;
        bitmap_property.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
        bitmap_property.dpiX = DPI;
        bitmap_property.dpiY = DPI;
        bitmap_property.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
        bitmap_property.colorContext = nullptr;
        m_device_context_D2D1->CreateBitmapFromDxgiSurface(surface.Get(), &bitmap_property, &m_RT_D2D1[i]);
    }

    return;
}

auto text_primitive_t::render_begin(std::size_t frame_index) -> void {
    m_device_D3D11on12->AcquireWrappedResources(m_RT_D3D11[frame_index].GetAddressOf(), 1);
    m_device_context_D2D1->SetTarget(m_RT_D2D1[frame_index].Get());
    m_device_context_D2D1->BeginDraw();
    return;
}

auto text_primitive_t::render_end(std::size_t frame_index) -> void {
    m_device_context_D2D1->EndDraw();
    m_device_D3D11on12->ReleaseWrappedResources(m_RT_D3D11[frame_index].GetAddressOf(), 1);
    m_device_context_D3D11->Flush();
    return;
}

text_primitive_t::text_primitive_t(engine_t* engine, std::wstring text, pos_2D text_pos, size_2D text_size
, pos_2D clip_pos, size_2D clip_size, size_1D size_font, color_t color, alignment_2D alignment)
: m_engine{ engine }, m_text{ text }, m_pos{ static_cast<float>(text_pos.x), static_cast<float>(text_pos.y) }, m_clip_pos{ clip_pos }, m_clip_size{ clip_size } {
    Microsoft::WRL::ComPtr<IDWriteTextFormat> format{};
    m_factory_DWrite->CreateTextFormat(L"Noto Sans TC", m_font_collection.Get()
    , DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, size_font.x, L"", &format);
    switch (alignment.x) {
    case alignment_x::left: {
        format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        break;
    }
    case alignment_x::center: {
        format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        break;
    }
    case alignment_x::right: {
        format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
        break;
    }
    }
    switch (alignment.y) {
    case alignment_y::top: {
        format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
        break;
    }
    case alignment_y::center: {
        format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        break;
    }
    case alignment_y::bottom: {
        format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
        break;
    }
    }
    m_factory_DWrite->CreateTextLayout(m_text.data(), m_text.size(), format.Get(), static_cast<float>(text_size.x), static_cast<float>(text_size.y), &m_layout);
    create_brush(color);
    return;
}

auto text_primitive_t::set_color(color_t color) -> void {
    create_brush(color);
    m_engine->redraw();
    return;
}

auto text_primitive_t::get_size() -> size_2D {
    DWRITE_TEXT_METRICS metric{};
    m_layout->GetMetrics(&metric);
    return size_2D{ static_cast<std::uint64_t>(metric.width), static_cast<std::uint64_t>(metric.height) };
}

auto text_primitive_t::render() -> void {
    D2D1_RECT_F clip_rect{};
    clip_rect.left = m_clip_pos.x;
    clip_rect.right = m_clip_pos.x + m_clip_size.x;
    clip_rect.top = m_clip_pos.y;
    clip_rect.bottom = m_clip_pos.y + m_clip_size.y;
    m_device_context_D2D1->PushAxisAlignedClip(&clip_rect, D2D1_ANTIALIAS_MODE_ALIASED);
    m_device_context_D2D1->DrawTextLayout(m_pos, m_layout.Get(), m_brush.Get(), D2D1_DRAW_TEXT_OPTIONS_NO_SNAP);
    m_device_context_D2D1->PopAxisAlignedClip();
    return;
}

#endif