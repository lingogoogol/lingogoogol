export module lgo.GUI.Vulkan:descriptor_set_layout;

import external.Vulkan;

export namespace lgo
{
    class descriptor_set_layout_t {
    public:
        descriptor_set_layout_t() = default;
        descriptor_set_layout_t(std::string name);

        auto binding_add
        (
            std::uint32_t binding,
            vk::DescriptorType type,
            std::uint32_t count,
            vk::ShaderStageFlagBits shader_visibility
        )
        -> void;
        auto serialize(vk::raii::Device& device) -> void;
        auto interface_get() -> vk::raii::DescriptorSetLayout&;
    private:
        std::string m_name{};
        std::vector<vk::DescriptorSetLayoutBinding> m_binding_arr{};
        vk::raii::DescriptorSetLayout m_interface{ nullptr };
    };
}
