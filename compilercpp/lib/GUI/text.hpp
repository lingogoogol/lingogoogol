#ifndef COMPILERCPP_LIB_GUI_TEXT
#define COMPILERCPP_LIB_GUI_TEXT

#include <vector>
#include <string>

#include "../header/wrl_client.h"
#include "../header/dwrite_3.h"
#include "../header/d3d11on12.h"
#include "../header/d3d12.h"
#include "../header/dxgi1_6.h"
#include "../header/Windows.h"
#include "../header/d2d1_3.h"

class text {
private:
    static Microsoft::WRL::ComPtr<IDWriteFactory5> m_factory_DWrite;
    static Microsoft::WRL::ComPtr<ID3D11On12Device> m_device_D3D11on12;
    static Microsoft::WRL::ComPtr<ID2D1DeviceContext> m_device_context_D2D1;
    static Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_device_context_D3D11;
    static std::vector<Microsoft::WRL::ComPtr<ID3D11Resource>> m_RT_D3D11;
    static std::vector<Microsoft::WRL::ComPtr<ID2D1Bitmap1>> m_RT_D2D1;
    static Microsoft::WRL::ComPtr<IDWriteTextFormat> m_format;

    std::wstring m_content{};
    D2D1_POINT_2F m_pos{};
    Microsoft::WRL::ComPtr<IDWriteTextLayout> m_layout{};
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_brush{};
public:
    static auto init(Microsoft::WRL::ComPtr<ID3D12Device2> device_D3D12, Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue
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
        m_factory_DWrite->CreateFontFileReference(L"NotoSans-VariableFont_wdth,wght.ttf", nullptr, &font_file);
        font_set_builder->AddFontFile(font_file.Get());
        Microsoft::WRL::ComPtr<IDWriteFontSet> font_set{};
        font_set_builder->CreateFontSet(&font_set);
        Microsoft::WRL::ComPtr<IDWriteFontCollection1> font_collection{};
        m_factory_DWrite->CreateFontCollectionFromFontSet(font_set.Get(), &font_collection);
        m_factory_DWrite->CreateTextFormat(L"Noto Sans", font_collection.Get()
        , DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 64.0f, L"en-us", &m_format);
        
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

    text() {}

    text(std::wstring content, float pos_x, float pos_y, float width, float height, float R, float G, float B)
    : m_content{ content }, m_pos{ pos_x, pos_y } {
        m_factory_DWrite->CreateTextLayout(m_content.data(), m_content.size(), m_format.Get(), width, height, &m_layout);
        m_device_context_D2D1->CreateSolidColorBrush({ R, G, B, 1.0f }, &m_brush);
        return;
    }

    auto render(std::size_t frame_index) -> void {
        m_device_D3D11on12->AcquireWrappedResources(m_RT_D3D11[frame_index].GetAddressOf(), 1);
        m_device_context_D2D1->SetTarget(m_RT_D2D1[frame_index].Get());
        m_device_context_D2D1->BeginDraw();
        D2D1_POINT_2F text_pos{};
        text_pos.x = 0.0f;
        text_pos.y = 0.0f;
        m_device_context_D2D1->DrawTextLayout(text_pos, m_layout.Get(), m_brush.Get(), D2D1_DRAW_TEXT_OPTIONS_NO_SNAP);
        m_device_context_D2D1->EndDraw();
        m_device_D3D11on12->ReleaseWrappedResources(m_RT_D3D11[frame_index].GetAddressOf(), 1);
        m_device_context_D3D11->Flush();
        return;
    }
};

Microsoft::WRL::ComPtr<IDWriteFactory5> text::m_factory_DWrite{};
Microsoft::WRL::ComPtr<ID3D11On12Device> text::m_device_D3D11on12{};
Microsoft::WRL::ComPtr<ID2D1DeviceContext> text::m_device_context_D2D1{};
Microsoft::WRL::ComPtr<ID3D11DeviceContext> text::m_device_context_D3D11{};
std::vector<Microsoft::WRL::ComPtr<ID3D11Resource>> text::m_RT_D3D11{};
std::vector<Microsoft::WRL::ComPtr<ID2D1Bitmap1>> text::m_RT_D2D1{};
Microsoft::WRL::ComPtr<IDWriteTextFormat> text::m_format{};

#endif