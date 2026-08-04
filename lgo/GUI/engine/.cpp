import external.Vulkan;
import external.GLFW;

namespace lgo {
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
        (*callback)(static_cast<std::uint16_t>(wparam));
        return;
    }

    auto engine_t::call_callback_char(std::function<void(wchar_t)>* callback, WPARAM wparam) -> void {
        (*callback)(static_cast<wchar_t>(wparam));
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
            push_transition_barrier(command_list, RT_current, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
            D3D12_CPU_DESCRIPTOR_HANDLE RTV_handle{ create_V_handle(ptr->m_RTV_heap->GetCPUDescriptorHandleForHeapStart()
            , ptr->m_RTV_size, ptr->m_frame_index) };
            clear_RT(command_list, RTV_handle, { 0.0f, 0.0f, 0.0f, 1.0f });
            D3D12_CPU_DESCRIPTOR_HANDLE DSV_handle{ ptr->m_DSV_heap->GetCPUDescriptorHandleForHeapStart() };
            command_list->ClearDepthStencilView(DSV_handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
            D3D12_VIEWPORT viewport{};
            viewport.TopLeftX = 0;
            viewport.TopLeftY = 0;
            viewport.Width = static_cast<FLOAT>(ptr->m_window_size.x);
            viewport.Height = static_cast<FLOAT>(ptr->m_window_size.y);
            viewport.MinDepth = D3D12_MIN_DEPTH;
            viewport.MaxDepth = D3D12_MAX_DEPTH;
            command_list->RSSetViewports(1, &viewport);
            command_list->OMSetRenderTargets(1, &RTV_handle, false, &DSV_handle);
            std::unique_lock rect_lock{ ptr->m_rect_mutex };
            rect_primitive_t::render(ptr->m_rect_primitive, command_list);
            rect_lock.unlock();
            push_transition_barrier(command_list, RT_current, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);
            ptr->m_command_queue.execute_list(command_list);

            ptr->m_buffer_fence_value[ptr->m_frame_index] = ptr->m_command_queue.set_fence();
            HRESULT present_result{ ptr->m_swap_chain->Present(0, ptr->m_tearing_supported ? DXGI_PRESENT_ALLOW_TEARING : 0) };
            handle_device_removed(present_result, ptr->m_device);
            ptr->m_frame_index = ptr->m_swap_chain->GetCurrentBackBufferIndex();
            ptr->m_command_queue.wait_fence(ptr->m_buffer_fence_value[ptr->m_frame_index]);
            EndPaint(window, &paint_info);
            return 0;
        }
        //...
        //handle_device_removed when resize
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

    VKAPI_ATTR auto VKAPI_CALL engine_t::debug_callback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
        vk::DebugUtilsMessageTypeFlagBitsEXT type,
        const vk::DebugUtilsMessengerCallbackDataEXT* data,
        void* engine_voidptr
    ) -> VkBool32 {
        std::string severity_str{};
        switch (severity) {
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose: {
            severity_str = "verbose";
            break;
        }
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo: {
            severity_str = "info";
            break;
        }
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning: {
            severity_str = "warning";
            break;
        }
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError: {
            severity_str = "error";
            break;
        }
        default: {
            severity_str = "unknown";
            break;
        }
        }
        std::string type_str{};
        switch (type) {
        case vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral: {
            severity_str = "general";
            break;
        }
        case vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation: {
            severity_str = "validation";
            break;
        }
        case vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance: {
            severity_str = "performance";
            break;
        }
        default: {
            severity_str = "unknown";
            break;
        }
        }
        log_file(
            "debug_message:\n"
            "    severity: " + severity_str + " (code: " + severity + ")\n"
            "    type: " + type_str + " (code: " + type + ")\n"
            "    message_id: " + data->pMessageIdName + " (code: " + data->messageIdNumber + ")\n"
            "    message: " + data->pMessage + "\n"
        );
        for (int i{ 0 }; i < data->objectCount; ++i) {
            log_file(
                "    object:\n"
                "        type: " "(code: " + data->pOjbects->objectType + ")\n"
                "        handle: " + data->pOjbects->objectHandle + "\n"
                "        name: " + data->pOjbects->pobjectName + "\n"
            );
        }
        if (severity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
            throw;
        }
        return vk::False;
    }

    engine_t::engine_t(
        HINSTANCE instance,
        size_2D window_size,
        std::string name
    ):
    m_window_size{ window_size }
    {
        //Create an instance.
        vk::ApplicationInfo app_info
        {
            .pApplicationName{ "app" },
            .applicationVersion{ Vulkan_make_version(0, 0, 5, 0) },
            .pEngineName{ "lgo" },
            .engineVersion{ Vulkan_make_version(0, 0, 5, 0) },
            .apiVersion{ Vulkan_make_version(0, 1, 4, 0) }
        };
        std::vector<const char*> layer
        {
            "VK_LAYER_KHRONOS_validation"
        };
        std::uint32_t glfw_extension_count{};
        const char** glfw_extension{ glfwGetRequiredInstanceExtensions(&glfw_extension_count) };
        std::vector<const char*> extension( glfw_extension, glfw_extension + glfw_extension_count );
        extension.push_back("VK_EXT_debug_utils");
        vk::InstanceCreateInfo instance_info
        {
            .pApplicationInfo{ &app_info },
            .enabledLayerCount{ static_cast<std::uint32_t>(layer.size()) },
            .ppEnabledLayerNames{ layer.data() }
            .enabledExtensionCount{ static_cast<std::uint32_t>(extension.size()) },
            .ppEnabledExtensionNames{ extension.data() }
        };
        m_instance = vk::raii::Instance{ m_context, instance_info };

        //Create a debug messenger.
        vk::DebugUtilsMessengerCreateInfoEXT messenger_info
        {
            .messageSeverity
            {
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose ||
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo ||
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning ||
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
            }
            .messageType
            {
                vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral ||
                vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation ||
                vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            }
            .pfnUserCallback{ &debug_callback },
            .pUserData{ this }
        };
        m_debug_messenger = m_instance.createDebugUtilsMessengerEXT(messenger_info);

        Microsoft::WRL::ComPtr<IDXGIFactory5> factory{ create_factory(true) };
        m_tearing_supported = check_tearing_support(factory);
        Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter{ create_adapter(factory) };
        m_device = create_device(adapter, name + ".m_device");
        m_info_queue = create_info_queue(m_device);
        m_command_queue.init(m_device, D3D12_COMMAND_LIST_TYPE_DIRECT, name + ".m_command_queue");
        
        //Create a window.
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        m_window = glfwCreateWindow(
            static_cast<int>(m_window_size.x),
            static_cast<int>(m_window_size.y),
            "window",
            nullptr,
            nullptr
        );

        SetWindowLongPtrW(m_window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        m_swap_chain = m_command_queue.create_swap_chain(factory, m_window, static_cast<UINT>(m_window_size.x)
        , static_cast<UINT>(m_window_size.y), DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_USAGE_RENDER_TARGET_OUTPUT, buffer_count, m_tearing_supported);
        m_RTV_heap = create_V_heap(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, buffer_count, name + ".m_RTV_heap");
        m_RTV_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        m_RT = create_RT(m_swap_chain, buffer_count);
        create_RTV(m_device, m_RT, m_RTV_size, m_RTV_heap);

        auto heap_property{ create_default_heap_property() };
        D3D12_RESOURCE_DESC DS_resource_description{};
        DS_resource_description.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        DS_resource_description.Alignment = 0;
        DS_resource_description.Width = static_cast<UINT>(window_size.x);
        DS_resource_description.Height = static_cast<UINT>(window_size.y);
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
        m_device->CreateCommittedResource(&heap_property, D3D12_HEAP_FLAG_NONE, &DS_resource_description
        , D3D12_RESOURCE_STATE_DEPTH_WRITE, &DS_clear_value, IID_PPV_ARGS(&m_DS));
        D3D12_set_name(m_DS, name + ".m_DS");
        m_DSV_heap = create_V_heap(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, name + ".m_DSV_heap");
        D3D12_DEPTH_STENCIL_VIEW_DESC DSV_description{};
        DSV_description.Format = DXGI_FORMAT_D32_FLOAT;
        DSV_description.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        DSV_description.Flags = D3D12_DSV_FLAG_NONE;
        DSV_description.Texture2D.MipSlice = 0;
        m_device->CreateDepthStencilView(m_DS.Get(), &DSV_description, m_DSV_heap->GetCPUDescriptorHandleForHeapStart());
        rect_primitive_t::init(m_device, m_window_size);

        m_frame_index = m_swap_chain->GetCurrentBackBufferIndex();
        m_buffer_fence_value.resize(buffer_count);
        m_initialized = true;
        D3D12_FEATURE_DATA_ROOT_SIGNATURE root_signature_version{};
        root_signature_version.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
        m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &root_signature_version, sizeof root_signature_version);
        if (root_signature_version.HighestVersion != D3D_ROOT_SIGNATURE_VERSION_1_1) {
            log_file("DirectX支持的根簽章版本太低\n");
        }
        ShowWindow(m_window, SW_SHOW);
        track_mouse_event();
        return;
    }

    engine_t::~engine_t() {
        flush();
        rect_primitive_t::uninit();
        glfwDestroyWindow(m_window);
        glfwTerminate();
        log_info_queue();
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

    auto engine_t::message_loop() -> bool {
        MSG message{};
        BOOL message_get_result{ GetMessageW(&message, NULL, 0, 0) };
        if (!message_get_result) {
            set_exit();
            return true;
        }
        if (message_get_result == -1) {
            log_file("讀取視窗訊息時發生錯誤\n");
            set_exit();
            return true;
        }
        glfwPollEvents();
        return glfwWindowShouldClose(m_window);
    }

    auto engine_t::log_info_queue() -> void {
        ::log_info_queue(m_info_queue);
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
        ScreenToClient(m_window, &cursor_pos);
        return pos_2D{ cursor_pos.x, cursor_pos.y };
    }

    auto engine_t::device_get() const -> Microsoft::WRL::ComPtr<ID3D12Device2> {
        return m_device;
    }

    auto engine_t::command_queue_get() -> command_queue_t& {
        return m_command_queue;
    }

    auto engine_t::add_rect(pos_2D pos, size_2D size, float depth, color_t color, std::string name) -> rect_primitive_t* {
        rect_primitive_t* out{ new rect_primitive_t{ this, pos, size, depth, color, name } };
        std::unique_lock lock{ m_rect_mutex };
        m_rect_primitive.emplace(out);
        redraw();
        return out;
    }

    auto engine_t::add_rect(pos_2D pos, size_2D size, float depth, pos_2D clip_pos, size_2D clip_size
    , const SRV_t& SRV, pos_2D texture_pos, size_2D texture_axis_x, size_2D texture_axis_y, std::string name) -> rect_primitive_t* {
        rect_primitive_t* out{ new rect_primitive_t{ this, pos, size, depth
        , clip_pos, clip_size, SRV, texture_pos, texture_axis_x, texture_axis_y, name } };
        std::unique_lock lock{ m_rect_mutex };
        m_rect_primitive.emplace(out);
        redraw();
        return out;
    }

    auto engine_t::remove_rect(rect_primitive_t* in) -> void {
        std::unique_lock lock{ m_rect_mutex };
        m_rect_primitive.erase(in);
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
}
