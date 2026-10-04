module;

#include <vulkan/vk_platform.h>
#include <glm/glm.hpp>

export module lgo.GUI.primitive:engine;

import std;

import external.Vulkan;
import external.GLFW;

import lgo.math.vec;
import lgo.GUI.Vulkan;

import :rect;

export namespace lgo
{
    class engine_t
    {
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
        vk::SurfaceFormat2KHR m_surface_format{};
        vk::Extent2D m_swap_chain_extent{};
        vk::raii::SwapchainKHR m_swap_chain{ nullptr };
        std::vector<vk::Image> m_swap_chain_image{};
        std::vector<vk::raii::ImageView> m_swap_chain_image_view{};
        image_t m_depth_image{};
        image_view_t m_depth_image_view{};
        pipeline_state_t m_pipeline_state{};
        vk::raii::CommandPool m_command_pool{ nullptr };
        vk::raii::CommandBuffer m_command_buffer{ nullptr };
        std::vector<vk::raii::Semaphore> m_semaphore_image{};
        std::vector<vk::raii::Semaphore> m_semaphore_draw{};
        vk::raii::Fence m_fence{ nullptr };
        std::uint64_t m_frame_number{ 0 };
        bool m_exit{ false };
        bool m_initialized{ false };
        bool m_tearing_supported{};
        std::recursive_mutex m_rect_mutex{};
        std::set<rect_primitive_t*> m_rect_primitive{};

        template<typename... t_arg>
        struct callback_set
        {
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
        static auto call_callback_none(std::function<void(void)>* callback) -> void;
        static auto call_callback_pos(std::function<void(pos_2D)>* callback, double x, double y) -> void;
        static auto call_callback_wheel
        (
            std::function<void(pos_2D, size_1D)>* callback,
            double x,
            double y,
            double window_x,
            double window_y
        )
        -> void;
        static auto call_callback_key(std::function<void(std::uint16_t)>* callback, int key) -> void;
        static auto call_callback_char(std::function<void(wchar_t)>* callback, char32_t cha) -> void;

        static auto GLFW_callback_cursor_pos(GLFWwindow* window, double xpos, double ypos) -> void;
        //for both cursor enter and leave
        static auto GLFW_callback_cursor_enter(GLFWwindow* window, int entered) -> void;
        static auto GLFW_callback_mouse_button(GLFWwindow* window, int button, int action, int mods) -> void;
        static auto GLFW_callback_wheel(GLFWwindow* window, double x, double y) -> void;
        static auto GLFW_callback_key(GLFWwindow* window, int key, int scancode, int action, int mods) -> void;
        static auto GLFW_callback_char(GLFWwindow* window, unsigned int codepoint) -> void;
        static VKAPI_ATTR auto VKAPI_CALL debug_callback
        (
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

        auto find_eligible_queue_family
        (
            std::vector<vk::QueueFamilyProperties2>& queue_family_arr,
            const vk::raii::PhysicalDevice& device_physical
        )
        -> std::ranges::iterator_t<std::vector<vk::QueueFamilyProperties2>>;
    public:
        engine_t(size_2D window_size, std::string name);
        engine_t(const engine_t&) = delete;
        ~engine_t();
        auto operator=(const engine_t&) = delete;

        auto message_loop() -> bool;
        auto draw() -> void;
        auto flush() -> void;
        auto log(std::uint32_t tab_count) const -> void;

        auto format_get() const -> vk::Format;
        auto set_exit() -> void;
        auto get_exit() -> bool;
        auto get_window_pos() -> pos_2D;
        auto get_window_size() -> const size_2D&;
        auto get_cursor_pos() -> pos_2D;
        auto device_get() -> vk::raii::Device&;
        auto device_physical_get() -> vk::raii::PhysicalDevice&;
        auto command_queue_get() -> vk::raii::Queue&;
        auto command_buffer_create() -> vk::raii::CommandBuffer;
        auto window_get() -> GLFWwindow*;

        using rect_t = rect_primitive_t::ctor_t;
        auto add_rect(const rect_t& rect) -> rect_primitive_t*;
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
