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
        std::string m_name{};
        
        template<typename t_callback_set, typename... t_in>
        static auto call_callback(t_callback_set& callback, t_in... in) -> void;
        static auto call_callback_none(std::function<void(int)>* callback, int entered) -> void;
        static auto call_callback_pos(std::function<void(pos_2D)>* callback, pos_2D pos) -> void;
        static auto call_callback_button(std::function<void(int, int, int)>* callback, int i, int j, int k) -> void;
        static auto call_callback_wheel(std::function<void(size_2D)>* callback, size_2D size) -> void;
        static auto call_callback_key
        (
            std::function<void(int, int, int)>* callback,
            int key,
            int action,
            int mods
        )
        -> void;
        static auto call_callback_char(std::function<void(char32_t)>* callback, char32_t cha) -> void;

        //This includes both cursor enter and cursor leave.
        static auto GLFW_callback_cursor_enter(GLFWwindow* window, int entered) -> void;
        static auto GLFW_callback_cursor_move(GLFWwindow* window, double x, double y) -> void;
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

        auto cursor_pos_get() const -> pos_2D;

        //This includes both cursor enter and cursor leave.
        using callback_mouse_enter_t = std::function<void(int)>;
        //This includes both cursor enter and cursor leave.
        auto callback_mouse_enter_add(callback_mouse_enter_t callback) -> callback_mouse_enter_t*;
        //This includes both cursor enter and cursor leave.
        auto callback_mouse_enter_remove(callback_mouse_enter_t* in) -> void;

        using callback_mouse_move_t = std::function<void(pos_2D)>;
        auto callback_mouse_move_add(callback_mouse_move_t callback) -> callback_mouse_move_t*;
        auto callback_mouse_move_remove(callback_mouse_move_t* in) -> void;

        using callback_mouse_button_t = std::function<void(int, int, int)>;
        auto callback_mouse_button_add(callback_mouse_button_t callback) -> callback_mouse_button_t*;
        auto callback_mouse_button_remove(callback_mouse_button_t* in) -> void;

        using callback_mouse_scroll_t = std::function<void(size_2D)>;
        auto callback_mouse_scroll_add(callback_mouse_scroll_t callback) -> callback_mouse_scroll_t*;
        auto callback_mouse_scroll_remove(callback_mouse_scroll_t* in) -> void;

        using callback_key_t = std::function<void(int, int, int)>;
        auto callback_key_add(callback_key_t callback) -> callback_key_t*;
        auto callback_key_remove(callback_key_t* in) -> void;

        using callback_cha_t = std::function<void(char32_t)>;
        auto callback_cha_add(callback_cha_t callback) -> callback_cha_t*;
        auto callback_cha_remove(callback_cha_t* in) -> void;
    private:
        template<typename t_callback>
        struct callback_set
        {
            std::set<t_callback*> m_effective{};
            std::set<t_callback*> m_pending{};
            bool m_calling_callback{ false };
        };

        callback_set<callback_mouse_enter_t> m_callback_mouse_enter{};
        callback_set<callback_mouse_move_t> m_callback_mouse_move{};
        callback_set<callback_mouse_button_t> m_callback_mouse_button{};
        callback_set<callback_mouse_scroll_t> m_callback_mouse_scroll{};
        callback_set<callback_key_t> m_callback_key{};
        callback_set<callback_cha_t> m_callback_cha{};
    };
}
