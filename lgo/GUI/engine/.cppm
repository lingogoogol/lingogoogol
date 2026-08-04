export lgo.GUI.engine;

import std;

import external.Vulkan;
import external.GLFW;

import lgo.math.vec;

import :rect;

#include "../directx/command_queue.hpp"
#include "../directx/command_list.hpp"
#include "../directx/create.hpp"
#include "../directx/init.hpp"

export namespace lgo {
    constexpr inline UINT buffer_count{ 0x2 };

    class engine_t {
    private:
        GLFWwindow* m_window{};
        pos_2D m_window_pos{};
        size_2D m_window_size{};

        vk::raii::Context m_context{};
        vk::raii::Instance m_instance{};
        vk::raii::DebugUtilsMessengerEXT m_debug_messenger{};
        vk::raii::PhysicalDevice m_device_physical{};
        bool m_exit{ false };
        bool m_initialized{ false };
        bool m_tearing_supported{};
        Microsoft::WRL::ComPtr<ID3D12Device2> m_device{};
        Microsoft::WRL::ComPtr<ID3D12InfoQueue> m_info_queue{};
        Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swap_chain{};
        std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_RT{};
        Microsoft::WRL::ComPtr<ID3D12Resource> m_DS{};
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DSV_heap{};
        command_queue_t m_command_queue{};
        std::vector<UINT64> m_buffer_fence_value{};
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RTV_heap{};
        UINT m_RTV_size{};
        UINT m_frame_index{};
        std::recursive_mutex m_rect_mutex{};
        std::set<rect_primitive_t*> m_rect_primitive{};

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
        std::string m_name{};
        
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
        engine_t(HINSTANCE instance, size_2D window_size, std::string name);
        engine_t(const engine_t&) = delete;
        ~engine_t();
        auto operator=(const engine_t&) = delete;

        auto flush() -> void;
        auto redraw() -> void;
        auto message_loop() -> bool;
        auto log_info_queue() -> void;

        auto set_exit() -> void;
        auto get_exit() -> bool;
        auto get_window_pos() -> pos_2D;
        auto get_window_size() -> size_2D;
        auto get_cursor_pos() -> pos_2D;
        auto device_get() const -> Microsoft::WRL::ComPtr<ID3D12Device2>;
        auto command_queue_get() -> command_queue_t&;

        auto add_rect(pos_2D pos, size_2D size, float depth, color_t color, std::string name) -> rect_primitive_t*;
        auto add_rect(pos_2D pos, size_2D size, float depth, pos_2D clip_pos, size_2D clip_size
        , const SRV_t& SRV, pos_2D texture_pos, size_2D texture_axis_x, size_2D texture_axis_y, std::string name) -> rect_primitive_t*;
        auto remove_rect(rect_primitive_t* in) -> void;

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
}
