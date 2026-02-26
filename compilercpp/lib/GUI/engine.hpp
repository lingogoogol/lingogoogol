#ifndef COMPILERCPP_LIB_GUI_ENGINE
#define COMPILERCPP_LIB_GUI_ENGINE

#include <vector>
#include <set>

#include "../header.hpp"

#include "stu.hpp"
#include "rect.hpp"
#include "text.hpp"

#include "../directx/command_queue_smart.hpp"
#include "../directx/command_list.hpp"
#include "../directx/create.hpp"
#include "../directx/init.hpp"

constexpr inline UINT buffer_count{ 0x2 };

class engine_t {
private:
    HWND m_window{};
    size_2D m_window_size{};
    bool m_initialized{ false };
    bool m_tearing_supported{};
    Microsoft::WRL::ComPtr<ID3D12InfoQueue> m_info_queue{};
    Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swap_chain{};
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_RT{};
    Microsoft::WRL::ComPtr<ID3D12Resource> m_DS{};
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DSV_heap{};
    command_queue_smart m_command_queue{};
    std::vector<UINT64> m_buffer_fence_value{};
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RTV_heap{};
    UINT m_RTV_size{};
    UINT m_frame_index{};
    std::set<rect_t*> m_rect{};
    std::set<text_t*> m_text{};

    static auto CALLBACK window_proc(HWND window, UINT message, WPARAM param_1, LPARAM param_2) -> LRESULT {
        engine_t* ptr{ reinterpret_cast<engine_t*>(GetWindowLongPtrW(window, GWLP_USERDATA)) };
        if (!ptr || !ptr->m_initialized) {
            return DefWindowProcW(window, message, param_1, param_2);
        }
        switch (message) {
        case WM_PAINT: {
            PAINTSTRUCT paint_info{};
            BeginPaint(window, &paint_info);
            Microsoft::WRL::ComPtr<ID3D12Resource> RT_current{ ptr->m_RT[ptr->m_frame_index] };
            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list{ ptr->m_command_queue.create_list() };
            push_transition_barrier(command_list, RT_current, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
            D3D12_CPU_DESCRIPTOR_HANDLE RTV_handle{ create_V_handle(ptr->m_RTV_heap->GetCPUDescriptorHandleForHeapStart()
            , ptr->m_RTV_size, ptr->m_frame_index) };
            clear_RT(command_list, RTV_handle, { 0.0f, 0.0f, 0.0f, 1.0f });
            D3D12_CPU_DESCRIPTOR_HANDLE DSV_handle{ ptr->m_DSV_heap->GetCPUDescriptorHandleForHeapStart() };
            command_list->ClearDepthStencilView(DSV_handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
            D3D12_VIEWPORT viewport{};
            viewport.TopLeftX = 0;
            viewport.TopLeftY = 0;
            viewport.Width = ptr->m_window_size.x;
            viewport.Height = ptr->m_window_size.y;
            viewport.MinDepth = D3D12_MIN_DEPTH;
            viewport.MaxDepth = D3D12_MAX_DEPTH;
            command_list->RSSetViewports(1, &viewport);
            D3D12_RECT scissor_rect{};
            scissor_rect.left = 0;
            scissor_rect.top = 0;
            scissor_rect.right = ptr->m_window_size.x;
            scissor_rect.bottom = ptr->m_window_size.y;
            command_list->RSSetScissorRects(1, &scissor_rect);
            command_list->OMSetRenderTargets(1, &RTV_handle, false, &DSV_handle);
            rect_t::render_begin(command_list);
            for (auto i{ ptr->m_rect.begin() }; i != ptr->m_rect.end(); ++i) {
                (*i)->render(command_list);
            }
            rect_t::render_end();
            ptr->m_command_queue.execute_list(command_list);
            text_t::render_begin(ptr->m_frame_index);
            for (auto i{ ptr->m_text.begin() }; i != ptr->m_text.end(); ++i) {
                (*i)->render();
            }
            text_t::render_end(ptr->m_frame_index);

            ptr->m_buffer_fence_value[ptr->m_frame_index] = ptr->m_command_queue.set_fence();
            ptr->m_swap_chain->Present(0, ptr->m_tearing_supported ? DXGI_PRESENT_ALLOW_TEARING : 0);
            ptr->m_frame_index = ptr->m_swap_chain->GetCurrentBackBufferIndex();
            ptr->m_command_queue.wait_fence(ptr->m_buffer_fence_value[ptr->m_frame_index]);
            EndPaint(window, &paint_info);
            return 0;
        }
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
        default: {
            return DefWindowProcW(window, message, param_1, param_2);
        }
        }
    }

    auto redraw() -> void {
        RedrawWindow(m_window, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
        return;
    }
public:
    engine_t(HINSTANCE instance, size_2D window_size): m_window_size{ window_size } {
        Microsoft::WRL::ComPtr<IDXGIFactory5> factory{ create_factory(true) };
        m_tearing_supported = check_tearing_support(factory);
        ATOM window_class{ create_window_class(L"window_class", &window_proc, instance
        , reinterpret_cast<HICON>(LoadImageW(NULL, IDI_APPLICATION, IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_SHARED))
        , reinterpret_cast<HICON>(LoadImageW(NULL, IDI_APPLICATION, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED))
        , reinterpret_cast<HCURSOR>(LoadImageW(NULL, IDC_ARROW, IMAGE_CURSOR, GetSystemMetrics(SM_CXCURSOR), GetSystemMetrics(SM_CYCURSOR), LR_SHARED))) };
        m_window = create_window(factory, window_class, L"compilercpp", WS_POPUP, m_window_size.x, m_window_size.y, instance);
        SetWindowLongPtrW(m_window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter{ create_adapter(factory) };
        Microsoft::WRL::ComPtr<ID3D12Device2> device{ create_device(adapter) };
        m_info_queue = create_info_queue(device);
        m_command_queue.init(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
        m_swap_chain = m_command_queue.create_swap_chain(factory, m_window, m_window_size.x
        , m_window_size.y, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_USAGE_RENDER_TARGET_OUTPUT, buffer_count, m_tearing_supported);
        m_RTV_heap = create_V_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, buffer_count);
        m_RTV_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        m_RT = create_RT(m_swap_chain, buffer_count);
        create_RTV(device, m_RT, m_RTV_size, m_RTV_heap);
        auto heap_property{ create_default_heap_property() };
        D3D12_RESOURCE_DESC DS_resource_description{};
        DS_resource_description.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        DS_resource_description.Alignment = 0;
        DS_resource_description.Width = window_size.x;
        DS_resource_description.Height = window_size.y;
        DS_resource_description.DepthOrArraySize = 1;
        DS_resource_description.MipLevels = 1;
        DS_resource_description.Format = DXGI_FORMAT_D32_FLOAT;
        DS_resource_description.SampleDesc.Count = 1;
        DS_resource_description.SampleDesc.Quality = 0;
        DS_resource_description.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        DS_resource_description.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        D3D12_CLEAR_VALUE DS_clear_value{};
        DS_clear_value.Format = DXGI_FORMAT_D32_FLOAT;
        DS_clear_value.DepthStencil.Depth = 1.0f;
        device->CreateCommittedResource(&heap_property, D3D12_HEAP_FLAG_NONE, &DS_resource_description
        , D3D12_RESOURCE_STATE_DEPTH_WRITE, &DS_clear_value, IID_PPV_ARGS(&m_DS));
        m_DSV_heap = create_V_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);
        D3D12_DEPTH_STENCIL_VIEW_DESC DSV_description{};
        DSV_description.Format = DXGI_FORMAT_D32_FLOAT;
        DSV_description.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        DSV_description.Flags = D3D12_DSV_FLAG_NONE;
        DSV_description.Texture2D.MipSlice = 0;
        device->CreateDepthStencilView(m_DS.Get(), &DSV_description, m_DSV_heap->GetCPUDescriptorHandleForHeapStart());
        text_t::init(device, m_command_queue.get(), m_window, m_RT);
        rect_t::init(device, m_window_size);
        m_frame_index = m_swap_chain->GetCurrentBackBufferIndex();
        m_buffer_fence_value.resize(buffer_count);
        m_initialized = true;
        D3D12_FEATURE_DATA_ROOT_SIGNATURE root_signature_version{};
        root_signature_version.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
        device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &root_signature_version, sizeof root_signature_version);
        if (root_signature_version.HighestVersion != D3D_ROOT_SIGNATURE_VERSION_1_1) {
            log_file("root_signature_version\r\n");
        }
        ShowWindow(m_window, SW_SHOW);
        return;
    }

    auto add_rect(const rect_t& in) -> rect_t* {
        rect_t* out{ new rect_t{ in } };
        m_rect.emplace(out);
        redraw();
        return out;
    }

    auto add_text(const text_t& in) -> text_t* {
        text_t* out{ new text_t{ in } };
        m_text.emplace(out);
        redraw();
        return out;
    }

    auto remove_rect(rect_t* in) -> void {
        m_rect.erase(in);
        m_command_queue.flush();
        delete in;
        redraw();
        return;
    }

    auto remove_text(text_t* in) -> void {
        m_text.erase(in);
        m_command_queue.flush();
        delete in;
        redraw();
        return;
    }

    ~engine_t() {
        m_command_queue.flush();
        for (auto i{ m_rect.begin() }; i != m_rect.end(); ++i) {
            delete *i;
        }
        for (auto i{ m_text.begin() }; i != m_text.end(); ++i) {
            delete *i;
        }
        log_info_queue(m_info_queue);
        return;
    }
};

#endif