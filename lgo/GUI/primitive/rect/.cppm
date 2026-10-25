module;

#include <glm/glm.hpp>

export module lgo.GUI.primitive:rect;

import std;

import external.Vulkan;

import lgo.math.vec;
import lgo.GUI.misc;
import lgo.GUI.Vulkan;

import :engine_decl;

export namespace lgo
{
    class rect_primitive_t
    {
        // The engine refreshes normalized vertices after the logical window size changes.
        friend class engine_t;
    private:
        struct solid_vertex_t
        {
            alignas(16) glm::vec3 pos{};
            alignas(16) glm::vec3 color{};
        };

        struct texture_vertex_t
        {
            alignas(16) glm::vec3 pos{};
            alignas(4) glm::uint32_t texture_index{};
            alignas(8) glm::vec2 texcoord{};
            alignas(16) glm::vec3 color{};
        };

        static engine_t* m_engine;
        static descriptor_set_layout_t m_solid_descriptor_set_layout;
        static descriptor_set_layout_t m_texture_descriptor_set_layout;
        static pipeline_state_t m_solid_pipeline_state;
        static pipeline_state_t m_texture_pipeline_state;
        static vk::raii::Sampler m_sampler;

        buffer_t m_vertex_buffer{};
        std::size_t m_vertex_buffer_size{};

        pos_2D m_pos{};
        size_2D m_size{};
        float m_depth{};
        color_t m_color{};
        vk::raii::DescriptorPool m_descriptor_pool{ nullptr };
        vk::raii::DescriptorSet m_descriptor_set{ nullptr };
        image_view_t* m_image_view{};
        pos_2D m_texture_pos{};
        size_2D m_texture_axis_x{};
        size_2D m_texture_axis_y{};
        std::recursive_mutex m_mutex{};
        bool m_texture_enable{};
        const pos_2D* m_clip_pos{};
        const size_2D* m_clip_size{};
        std::string m_name{};

        constexpr static std::size_t vertex_data_len{ 4 };

        auto release_gpu() noexcept -> void;
        auto create_gpu() -> void;
        auto create_vertex_buffer() -> void;
        auto upload_vertex_data() -> void;
        template<typename t_type>
        auto upload_vertex_data(const t_type& vertex_data) -> void;
    public:
        static auto init(engine_t* engine) -> void;
        static auto uninit() -> void;
        static auto render
        (
            const std::set<rect_primitive_t*>& rect,
            vk::raii::CommandBuffer& command_buffer
        )
        -> void;
        
        struct ctor_t
        {
            pos_2D pos{};
            size_2D size{};
            float depth{};
            pos_2D* clip_pos{};
            size_2D* clip_size{};
            color_t color{};
            image_view_t* image_view{};
            pos_2D texture_pos{};
            size_2D texture_axis_x{};
            size_2D texture_axis_y{};
            std::string name{};
        };
        rect_primitive_t() = default;
        rect_primitive_t(const ctor_t& ctor);
        
        auto set_pos(pos_2D pos) -> void;
        auto set_size(size_2D size) -> void;
        auto set_color(color_t color) -> void;
        auto set_image_view(image_view_t& image_view) -> void;
        auto color_enable() -> void;
        auto texture_enable() -> void;
        auto inside(pos_2D pos) -> bool;
        auto log(std::uint32_t tab_count) const -> void;
    private:
        auto render(vk::raii::CommandBuffer& command_buffer) -> void;
    };
}
