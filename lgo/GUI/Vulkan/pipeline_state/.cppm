export module lgo.GUI.Vulkan:pipeline_state;

import external.Vulkan;

import :shader;
import :descriptor_set_layout;

export namespace lgo
{
    class pipeline_state_t
    {
    public:
        struct ctor_t
        {
            std::string name{};
            descriptor_set_layout_t* descriptor_set_layout{};
            vk::PrimitiveTopology topology{};
            vk::Format surface_format{};
            vk::Format depth_format{ vk::Format::eD32Sfloat };
            std::uint32_t push_constant_size{};
        };
        
        pipeline_state_t() = default;
        pipeline_state_t(const ctor_t& ctor);
        auto init(const ctor_t& ctor) -> void;

        auto shader_add(shader_t& shader) -> void;
        auto vertex_binding_add
        (
            std::uint32_t binding,
            std::size_t stride,
            vk::VertexInputRate input_rate
        )
        -> void;
        auto vertex_attribute_add
        (
            std::uint32_t location,
            std::uint32_t binding,
            vk::Format format,
            std::uint32_t offset
        )
        -> void;
        auto serialize(vk::raii::Device& device) -> void;

        auto interface_get() -> vk::raii::Pipeline&;
        auto layout_get() -> vk::raii::PipelineLayout&;
    private:
        std::string m_name{};
        vk::PipelineInputAssemblyStateCreateInfo m_input_assembly{};
        descriptor_set_layout_t* m_descriptor_set_layout{};
        std::vector<shader_t> m_shader{};
        std::vector<vk::VertexInputBindingDescription> m_vertex_binding{};
        std::vector<vk::VertexInputAttributeDescription> m_vertex_attribute{};
        vk::Format m_surface_format{};
        vk::Format m_depth_format{ vk::Format::eD32Sfloat };
        std::uint32_t m_push_constant_size{};
        vk::raii::PipelineLayout m_pipeline_layout{ nullptr };
        vk::raii::Pipeline m_interface{ nullptr };
    };
}
