module;

#include <vulkan/vk_platform.h>

export module lgo.GUI.engine;

import std;

import external.Vulkan;
import external.GLFW;

import lgo.math.vec;

export namespace lgo {
    class engine_t {
    private:
        GLFWwindow* m_window{};
        pos_2D m_window_pos{};
        size_2D m_window_size{};

        vk::raii::Context m_context{};
        vk::raii::Instance m_instance{ nullptr };
        vk::raii::DebugUtilsMessengerEXT m_debug_messenger{ nullptr };
        vk::raii::SurfaceKHR m_surface{ nullptr };
        vk::raii::PhysicalDevice m_device_physical{ nullptr };
        vk::raii::Device m_device{ nullptr };
        vk::raii::Queue m_command_queue{ nullptr };
        vk::raii::SwapchainKHR m_swap_chain{ nullptr };
        std::vector<vk::Image> m_swap_chain_image{};
        std::vector<vk::raii::ImageView> m_swap_chain_image_view{};
        vk::raii::PipelineLayout m_pipeline_layout{ nullptr };
        vk::raii::Pipeline m_pipeline{ nullptr };
        vk::raii::CommandPool m_command_pool{ nullptr };
        vk::raii::CommandBuffer m_command_buffer{ nullptr };
        vk::raii::Semaphore m_semaphore_image{ nullptr };
        vk::raii::Semaphore m_semaphore_draw{ nullptr };
        vk::raii::Fence m_fence{ nullptr };

        bool m_exit{ false };
        bool m_initialized{ false };
        bool m_tearing_supported{};
        std::recursive_mutex m_rect_mutex{};
        //std::set<rect_primitive_t*> m_rect_primitive{};//...

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
        
        template<typename t_callback_set, typename t_caller, typename... t_in>
        static auto call_callback(const t_caller& caller, t_callback_set& callback, t_in... in) -> void;
        static VKAPI_ATTR auto VKAPI_CALL debug_callback(
            vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
            vk::DebugUtilsMessageTypeFlagsEXT type,
            const vk::DebugUtilsMessengerCallbackDataEXT* data,
            void* engine_voidptr
        )
        -> vk::Bool32;

        auto transition_image_layout
        (
            std::uint32_t image_index,
            vk::ImageLayout old_layout,
            vk::ImageLayout new_layout,
            vk::AccessFlags2 src_access_mask,
            vk::AccessFlags2 dest_access_mask,
            vk::PipelineStageFlags2 src_stage_mask,
            vk::PipelineStageFlags2 dest_stage_mask
        )
        -> void;
    public:
        engine_t(size_2D window_size, std::string name);
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
        auto device_get() const -> vk::raii::Device;
        auto command_queue_get() -> vk::raii::Queue;

        /*auto add_rect(pos_2D pos, size_2D size, float depth, color_t color, std::string name) -> rect_primitive_t*;
        auto add_rect(pos_2D pos, size_2D size, float depth, pos_2D clip_pos, size_2D clip_size
        , const SRV_t& SRV, pos_2D texture_pos, size_2D texture_axis_x, size_2D texture_axis_y, std::string name) -> rect_primitive_t*;
        auto remove_rect(rect_primitive_t* in) -> void;*///...

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
