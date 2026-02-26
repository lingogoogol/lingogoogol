#ifndef COMPILERCPP_GUI_PRIMITIVE_TEXT_IMPL
#define COMPILERCPP_GUI_PRIMITIVE_TEXT_IMPL

#include "text_def.hpp"
#include "engine_def.hpp"

auto text_primitive_t::create_layout() -> void {
    std::unique_lock lock{ m_mutex };
    m_factory_DWrite->CreateTextLayout(m_text.data(), static_cast<UINT32>(m_text.size()), m_format.Get()
    , static_cast<float>(m_text_size.x), static_cast<float>(m_text_size.y), &m_layout);
    return;
}

auto text_primitive_t::create_brush(color_t color) -> void {
    std::unique_lock lock{ m_mutex };
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

    m_factory_DWrite->CreateInMemoryFontFileLoader(&m_font_file_loader);
    m_factory_DWrite->RegisterFontFileLoader(m_font_file_loader.Get());
    Microsoft::WRL::ComPtr<IDWriteFontFile> font_file{};
    std::string font_file_data{ get_resource(256, 3) };
    m_font_file_loader->CreateInMemoryFontFileReference(m_factory_DWrite.Get(), font_file_data.data(), static_cast<UINT32>(font_file_data.size()), nullptr, &font_file);
    Microsoft::WRL::ComPtr<IDWriteFontSetBuilder1> font_set_builder{};
    m_factory_DWrite->CreateFontSetBuilder(&font_set_builder);
    font_set_builder->AddFontFile(font_file.Get());
    Microsoft::WRL::ComPtr<IDWriteFontSet> font_set{};
    font_set_builder->CreateFontSet(&font_set);
    m_factory_DWrite->CreateFontCollectionFromFontSet(font_set.Get(), &m_font_collection);
    
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
        bitmap_property.dpiX = static_cast<FLOAT>(DPI);
        bitmap_property.dpiY = static_cast<FLOAT>(DPI);
        bitmap_property.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
        bitmap_property.colorContext = nullptr;
        m_device_context_D2D1->CreateBitmapFromDxgiSurface(surface.Get(), &bitmap_property, &m_RT_D2D1[i]);
    }

    return;
}

auto text_primitive_t::uninit() -> void {
    m_factory_DWrite->UnregisterFontFileLoader(m_font_file_loader.Get());
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
, pos_2D clip_pos, size_2D clip_size, size_1D size_font, color_t color, alignment_2D alignment, bool wrap)
: m_engine{ engine }, m_text{ text }, m_text_pos{ text_pos }, m_text_size{ text_size }, m_clip_pos{ clip_pos }, m_clip_size{ clip_size } {
    m_factory_DWrite->CreateTextFormat(L"Noto Sans TC", m_font_collection.Get()
    , DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, static_cast<FLOAT>(size_font.x), L"", &m_format);
    switch (alignment.x) {
    case alignment_x::left: {
        m_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        break;
    }
    case alignment_x::center: {
        m_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        break;
    }
    case alignment_x::right: {
        m_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
        break;
    }
    }
    switch (alignment.y) {
    case alignment_y::top: {
        m_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
        break;
    }
    case alignment_y::center: {
        m_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        break;
    }
    case alignment_y::bottom: {
        m_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
        break;
    }
    }
    if (!wrap) {
        m_format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    }
    create_layout();
    create_brush(color);
    return;
}

auto text_primitive_t::get_text() -> std::wstring {
    std::unique_lock lock{ m_mutex };
    return m_text;
}

auto text_primitive_t::set_text(std::wstring text) -> void {
    std::unique_lock lock{ m_mutex };
    m_text = text;
    create_layout();
    return;
}

auto text_primitive_t::get_text_pos() -> pos_2D {
    std::unique_lock lock{ m_mutex };
    return m_text_pos;
}

auto text_primitive_t::set_text_pos(pos_2D text_pos) -> void {
    std::unique_lock lock{ m_mutex };
    m_text_pos = text_pos;
    create_layout();
    return;
}

auto text_primitive_t::get_text_size() -> size_2D {
    std::unique_lock lock{ m_mutex };
    return m_text_size;
}

auto text_primitive_t::set_text_size(size_2D text_size) -> void {
    std::unique_lock lock{ m_mutex };
    m_text_size = text_size;
    create_layout();
    return;
}

auto text_primitive_t::get_clip_pos() -> pos_2D {
    std::unique_lock lock{ m_mutex };
    return m_clip_pos;
}

auto text_primitive_t::set_clip_pos(pos_2D clip_pos) -> void {
    std::unique_lock lock{ m_mutex };
    m_clip_pos = clip_pos;
    return;
}

auto text_primitive_t::get_clip_size() -> size_2D {
    std::unique_lock lock{ m_mutex };
    return m_clip_size;
}

auto text_primitive_t::set_clip_size(size_2D clip_size) -> void {
    std::unique_lock lock{ m_mutex };
    m_clip_size = clip_size;
    return;
}

auto text_primitive_t::set_color(color_t color) -> void {
    std::unique_lock lock{ m_mutex };
    create_brush(color);
    m_engine->redraw();
    return;
}

auto text_primitive_t::get_render_size() -> size_2D {
    std::unique_lock lock{ m_mutex };
    DWRITE_TEXT_METRICS metric{};
    m_layout->GetMetrics(&metric);
    return size_2D{ static_cast<std::int64_t>(metric.width), static_cast<std::int64_t>(metric.height) };
}

auto text_primitive_t::render() -> void {
    std::unique_lock lock{ m_mutex };
    D2D1_RECT_F clip_rect{};
    clip_rect.left = static_cast<FLOAT>(m_clip_pos.x);
    clip_rect.right = static_cast<FLOAT>(m_clip_pos.x + m_clip_size.x);
    clip_rect.top = static_cast<FLOAT>(m_clip_pos.y);
    clip_rect.bottom = static_cast<FLOAT>(m_clip_pos.y + m_clip_size.y);
    m_device_context_D2D1->PushAxisAlignedClip(&clip_rect, D2D1_ANTIALIAS_MODE_ALIASED);
    D2D1_POINT_2F text_pos{ static_cast<FLOAT>(m_text_pos.x), static_cast<FLOAT>(m_text_pos.y) };
    m_device_context_D2D1->DrawTextLayout(text_pos, m_layout.Get(), m_brush.Get(), D2D1_DRAW_TEXT_OPTIONS_NO_SNAP);
    m_device_context_D2D1->PopAxisAlignedClip();
    return;
}

#endif