#ifndef COMPILERCPP_LIB_GUI_PRIMITIVE_ENGINE_DEF
#define COMPILERCPP_LIB_GUI_PRIMITIVE_ENGINE_DEF

#include <vector>
#include <memory>
#include <set>
#include <map>
#include <functional>
#include <cstdint>

#include "../header.hpp"

#include "engine_decl.hpp"
#include "rect_def.hpp"
#include "text_def.hpp"

#include "../stu.hpp"
#include "../directx/command_queue_smart.hpp"
#include "../directx/command_list.hpp"
#include "../directx/create.hpp"
#include "../directx/init.hpp"

constexpr inline UINT buffer_count{ 0x2 };

class engine_t {
private:
    HWND m_window{};
    pos_2D m_window_pos{};
    size_2D m_window_size{};
    bool m_exit{ false };
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
    std::set<rect_primitive_t*> m_rect_primitive{};
    std::set<text_primitive_t*> m_text_primitive{};

    template<typename... t_arg>
    struct callback_set {
        std::set<std::function<void(t_arg...)>*> m_effective{};
        std::set<std::function<void(t_arg...)>*> m_pending{};
        bool m_calling_callback{ false };
    };
    callback_set<pos_2D> m_mouse_move{};
    callback_set<> m_mouse_leave{};
    callback_set<pos_2D> m_mouse_left_click{};
    callback_set<pos_2D> m_mouse_left_release{};
    callback_set<pos_2D, size_1D> m_mouse_scroll{};
    callback_set<std::uint16_t> m_key_down{};
    callback_set<wchar_t> m_charw{};
    
    auto track_mouse_event() -> void;
    template<typename t_callback_set, typename t_caller, typename... t_in>
    static auto call_callback(const t_caller& caller, t_callback_set& callback, t_in... in) -> void;
    static auto call_callback_none(std::function<void(void)>* callback) -> void;
    static auto call_callback_pos(std::function<void(pos_2D)>* callback, LPARAM lparam) -> void;
    static auto call_callback_wheel(std::function<void(pos_2D, size_1D)>* callback, LPARAM lparam, WPARAM wparam, pos_2D window_pos) -> void;
    static auto call_callback_key(std::function<void(std::uint16_t)>* callback, WPARAM wparam) -> void;
    static auto call_callback_char(std::function<void(wchar_t)>* callback, WPARAM wparam) -> void;
    static auto CALLBACK window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT;
public:
    engine_t(HINSTANCE instance, size_2D window_size);
    engine_t(const engine_t&) = delete;
    ~engine_t();
    auto operator=(const engine_t&) = delete;

    auto flush() -> void;
    auto redraw() -> void;
    auto set_exit() -> void;
    auto get_exit() -> bool;
    auto get_window_pos() -> pos_2D;
    auto get_window_size() -> size_2D;
    auto get_cursor_pos() -> pos_2D;

    auto add_rect(pos_2D pos, size_2D size, float depth, color_t color) -> rect_primitive_t*;
    auto remove_rect(rect_primitive_t* in) -> void;
    auto add_text(std::wstring content, pos_2D text_pos, size_2D text_size
    , pos_2D clip_pos, size_2D clip_size, size_1D size_font, color_t color, alignment_2D alignment) -> text_primitive_t*;
    auto remove_text(text_primitive_t* in) -> void;

    auto add_mouse_move(std::function<void(pos_2D)> callback) -> std::function<void(pos_2D)>*;
    auto remove_mouse_move(std::function<void(pos_2D)>* in) -> void;
    auto add_mouse_leave(std::function<void(void)> callback) -> std::function<void(void)>*;
    auto remove_mouse_leave(std::function<void(void)>* in) -> void;
    auto add_mouse_left_click(std::function<void(pos_2D)> callback) -> std::function<void(pos_2D)>*;
    auto remove_mouse_left_click(std::function<void(pos_2D)>* in) -> void;
    auto add_mouse_left_release(std::function<void(pos_2D)> callback) -> std::function<void(pos_2D)>*;
    auto remove_mouse_left_release(std::function<void(pos_2D)>* in) -> void;
    auto add_mouse_scroll(std::function<void(pos_2D, size_1D)> callback) -> std::function<void(pos_2D, size_1D)>*;
    auto remove_mouse_scroll(std::function<void(pos_2D, size_1D)>* in) -> void;
    auto add_key_down(std::function<void(std::uint16_t)> callback) -> std::function<void(std::uint16_t)>*;
    auto remove_key_down(std::function<void(std::uint16_t)>* in) -> void;
    auto add_charw(std::function<void(wchar_t)> callback) -> std::function<void(wchar_t)>*;
    auto remove_charw(std::function<void(wchar_t)>* in) -> void;
};

auto engine_t::track_mouse_event() -> void {
    TRACKMOUSEEVENT stu{};
    stu.cbSize = sizeof stu;
    stu.dwFlags = TME_LEAVE;
    stu.hwndTrack = m_window;
    TrackMouseEvent(&stu);
    return;
}

template<typename t_callback_set, typename t_caller, typename... t_in>
auto engine_t::call_callback(const t_caller& caller, t_callback_set& callback, t_in... in) -> void {
    callback.m_calling_callback = true;
    static std::uint64_t depth{ 0 };
    ++depth;
    typename decltype(callback.m_effective)::key_type current{};
    for (auto i{ callback.m_effective.begin() }; i != callback.m_effective.end(); i = callback.m_effective.upper_bound(current)) {
        current = *i;
        caller(current, in...);
    }
    --depth;
    if (!depth) {
        callback.m_effective.insert(callback.m_pending.begin(), callback.m_pending.end());
        callback.m_pending.clear();
    }
    callback.m_calling_callback = false;
    return;
}

auto engine_t::call_callback_none(std::function<void(void)>* callback) -> void {
    (*callback)();
    return;
}

auto engine_t::call_callback_pos(std::function<void(pos_2D)>* callback, LPARAM lparam) -> void {
    (*callback)(pos_2D{ GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) });
    return;
}

auto engine_t::call_callback_wheel(std::function<void(pos_2D, size_1D)>* callback, LPARAM lparam, WPARAM wparam, pos_2D window_pos) -> void {
    (*callback)(pos_2D{ GET_X_LPARAM(lparam) - window_pos.x, GET_Y_LPARAM(lparam) - window_pos.y }, size_1D{ GET_WHEEL_DELTA_WPARAM(wparam) / 120 });
    return;
}

auto engine_t::call_callback_key(std::function<void(std::uint16_t)>* callback, WPARAM wparam) -> void {
    (*callback)(wparam);
    return;
}

auto engine_t::call_callback_char(std::function<void(wchar_t)>* callback, WPARAM wparam) -> void {
    (*callback)(wparam);
    return;
}

auto CALLBACK engine_t::window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT {
    engine_t* ptr{ reinterpret_cast<engine_t*>(GetWindowLongPtrW(window, GWLP_USERDATA)) };
    if (!ptr || !ptr->m_initialized) {
        return DefWindowProcW(window, message, wparam, lparam);
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
        rect_primitive_t::render_begin(command_list);
        for (auto i{ ptr->m_rect_primitive.begin() }; i != ptr->m_rect_primitive.end(); ++i) {
            (*i)->render(command_list);
        }
        rect_primitive_t::render_end();
        ptr->m_command_queue.execute_list(command_list);
        text_primitive_t::render_begin(ptr->m_frame_index);
        for (auto i{ ptr->m_text_primitive.begin() }; i != ptr->m_text_primitive.end(); ++i) {
            (*i)->render();
        }
        text_primitive_t::render_end(ptr->m_frame_index);

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
    case WM_MOUSEMOVE: {
        ptr->track_mouse_event();
        call_callback(call_callback_pos, ptr->m_mouse_move, lparam);
        return 0;
    }
    case WM_MOUSELEAVE: {
        call_callback(call_callback_none, ptr->m_mouse_leave);
        return 0;
    }
    case WM_LBUTTONDOWN: {
        call_callback(call_callback_pos, ptr->m_mouse_left_click, lparam);
        return 0;
    }
    case WM_LBUTTONUP: {
        call_callback(call_callback_pos, ptr->m_mouse_left_release, lparam);
        return 0;
    }
    case WM_MOUSEWHEEL: {
        call_callback(call_callback_wheel, ptr->m_mouse_scroll, lparam, wparam, ptr->m_window_pos);
        return 0;
    }
    case WM_KEYDOWN: {
        call_callback(call_callback_key, ptr->m_key_down, wparam);
        return 0;
    }
    case WM_CHAR: {
        call_callback(call_callback_char, ptr->m_charw, wparam);
        return 0;
    }
    default: {
        return DefWindowProcW(window, message, wparam, lparam);
    }
    }
}

engine_t::engine_t(HINSTANCE instance, size_2D window_size): m_window_size{ window_size } {
    Microsoft::WRL::ComPtr<IDXGIFactory5> factory{ create_factory(true) };
    m_tearing_supported = check_tearing_support(factory);
    ATOM window_class{ create_window_class(L"window_class", &window_proc, instance
    , reinterpret_cast<HICON>(LoadImageW(NULL, IDI_APPLICATION, IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_SHARED))
    , reinterpret_cast<HICON>(LoadImageW(NULL, IDI_APPLICATION, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED))
    , reinterpret_cast<HCURSOR>(LoadImageW(NULL, IDC_ARROW, IMAGE_CURSOR, GetSystemMetrics(SM_CXCURSOR), GetSystemMetrics(SM_CYCURSOR), LR_SHARED))) };
    m_window = create_window(factory, window_class, L"compilercpp", WS_POPUP, m_window_size.x, m_window_size.y, instance, &m_window_pos);
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
    text_primitive_t::init(device, m_command_queue.get(), m_window, m_RT);
    rect_primitive_t::init(device, m_window_size);
    m_frame_index = m_swap_chain->GetCurrentBackBufferIndex();
    m_buffer_fence_value.resize(buffer_count);
    m_initialized = true;
    D3D12_FEATURE_DATA_ROOT_SIGNATURE root_signature_version{};
    root_signature_version.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &root_signature_version, sizeof root_signature_version);
    if (root_signature_version.HighestVersion != D3D_ROOT_SIGNATURE_VERSION_1_1) {
        log_file("root_signature_version\n");
    }
    ShowWindow(m_window, SW_SHOW);
    track_mouse_event();
    return;
}

engine_t::~engine_t() {
    flush();
    log_info_queue(m_info_queue);
    return;
}

auto engine_t::flush() -> void {
    m_command_queue.flush();
    return;
}

auto engine_t::redraw() -> void {
    InvalidateRect(m_window, nullptr, false);
    return;
}

auto engine_t::set_exit() -> void {
    m_exit = true;
    return;
}

auto engine_t::get_exit() -> bool {
    return m_exit;
}

auto engine_t::get_window_pos() -> pos_2D {
    return m_window_pos;
}

auto engine_t::get_window_size() -> size_2D {
    return m_window_size;
}

auto engine_t::get_cursor_pos() -> pos_2D {
    POINT cursor_pos{};
    GetCursorPos(&cursor_pos);
    return pos_2D{ cursor_pos.x, cursor_pos.y };
}

auto engine_t::add_rect(pos_2D pos, size_2D size, float depth, color_t color) -> rect_primitive_t* {
    rect_primitive_t* out{ new rect_primitive_t{ this, pos, size, depth, color } };
    m_rect_primitive.emplace(out);
    redraw();
    return out;
}

auto engine_t::remove_rect(rect_primitive_t* in) -> void {
    m_rect_primitive.erase(in);
    flush();
    delete in;
    redraw();
    return;
}

auto engine_t::add_text(std::wstring content, pos_2D text_pos, size_2D text_size
, pos_2D clip_pos, size_2D clip_size, size_1D size_font, color_t color, alignment_2D alignment) -> text_primitive_t* {
    text_primitive_t* out{ new text_primitive_t{ this, content, text_pos, text_size, clip_pos, clip_size, size_font, color, alignment } };
    m_text_primitive.emplace(out);
    redraw();
    return out;
}

auto engine_t::remove_text(text_primitive_t* in) -> void {
    m_text_primitive.erase(in);
    flush();
    delete in;
    redraw();
    return;
}

auto engine_t::add_mouse_move(std::function<void(pos_2D)> callback) -> std::function<void(pos_2D)>* {
    std::function<void(pos_2D)>* out{ new std::function<void(pos_2D)>{ callback } };
    if (m_mouse_move.m_calling_callback) {
        m_mouse_move.m_pending.emplace(out);
    }
    else {
        m_mouse_move.m_effective.emplace(out);
    }
    return out;
}

auto engine_t::remove_mouse_move(std::function<void(pos_2D)>* in) -> void {
    if (!m_mouse_move.m_effective.erase(in)) {
        m_mouse_move.m_pending.erase(in);
    }
    delete in;
    return;
}

auto engine_t::add_mouse_leave(std::function<void(void)> callback) -> std::function<void(void)>* {
    std::function<void(void)>* out{ new std::function<void(void)>{ callback } };
    if (m_mouse_leave.m_calling_callback) {
        m_mouse_leave.m_pending.emplace(out);
    }
    else {
        m_mouse_leave.m_effective.emplace(out);
    }
    return out;
}

auto engine_t::remove_mouse_leave(std::function<void(void)>* in) -> void {
    if (!m_mouse_leave.m_effective.erase(in)) {
        m_mouse_leave.m_pending.erase(in);
    }
    delete in;
    return;
}

auto engine_t::add_mouse_left_click(std::function<void(pos_2D)> callback) -> std::function<void(pos_2D)>* {
    std::function<void(pos_2D)>* out{ new std::function<void(pos_2D)>{ callback } };
    if (m_mouse_left_click.m_calling_callback) {
        m_mouse_left_click.m_pending.emplace(out);
    }
    else {
        m_mouse_left_click.m_effective.emplace(out);
    }
    return out;
}

auto engine_t::remove_mouse_left_click(std::function<void(pos_2D)>* in) -> void {
    if (!m_mouse_left_click.m_effective.erase(in)) {
        m_mouse_left_click.m_pending.erase(in);
    }
    delete in;
    return;
}

auto engine_t::add_mouse_left_release(std::function<void(pos_2D)> callback) -> std::function<void(pos_2D)>* {
    std::function<void(pos_2D)>* out{ new std::function<void(pos_2D)>{ callback } };
    if (m_mouse_left_release.m_calling_callback) {
        m_mouse_left_release.m_pending.emplace(out);
    }
    else {
        m_mouse_left_release.m_effective.emplace(out);
    }
    return out;
}

auto engine_t::remove_mouse_left_release(std::function<void(pos_2D)>* in) -> void {
    if (!m_mouse_left_release.m_effective.erase(in)) {
        m_mouse_left_release.m_pending.erase(in);
    }
    delete in;
    return;
}

auto engine_t::add_mouse_scroll(std::function<void(pos_2D, size_1D)> callback) -> std::function<void(pos_2D, size_1D)>* {
    std::function<void(pos_2D, size_1D)>* out{ new std::function<void(pos_2D, size_1D)>{ callback } };
    if (m_mouse_scroll.m_calling_callback) {
        m_mouse_scroll.m_pending.emplace(out);
    }
    else {
        m_mouse_scroll.m_effective.emplace(out);
    }
    return out;
}

auto engine_t::remove_mouse_scroll(std::function<void(pos_2D, size_1D)>* in) -> void {
    if (!m_mouse_scroll.m_effective.erase(in)) {
        m_mouse_scroll.m_pending.erase(in);
    }
    delete in;
    return;
}

auto engine_t::add_key_down(std::function<void(std::uint16_t)> callback) -> std::function<void(std::uint16_t)>* {
    std::function<void(std::uint16_t)>* out{ new std::function<void(std::uint16_t)>{ callback } };
    if (m_key_down.m_calling_callback) {
        m_key_down.m_pending.emplace(out);
    }
    else {
        m_key_down.m_effective.emplace(out);
    }
    return out;
}

auto engine_t::remove_key_down(std::function<void(std::uint16_t)>* in) -> void {
    if (!m_key_down.m_effective.erase(in)) {
        m_key_down.m_pending.erase(in);
    }
    delete in;
    return;
}

auto engine_t::add_charw(std::function<void(wchar_t)> callback) -> std::function<void(wchar_t)>* {
    std::function<void(wchar_t)>* out{ new std::function<void(wchar_t)>{ callback } };
    if (m_charw.m_calling_callback) {
        m_charw.m_pending.emplace(out);
    }
    else {
        m_charw.m_effective.emplace(out);
    }
    return out;
}

auto engine_t::remove_charw(std::function<void(wchar_t)>* in) -> void {
    if (!m_charw.m_effective.erase(in)) {
        m_charw.m_pending.erase(in);
    }
    delete in;
    return;
}

#endif