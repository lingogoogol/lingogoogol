module lgo.GUI.Vulkan;

import std;

import external.Vulkan;

namespace lgo
{
    descriptor_set_layout_t::descriptor_set_layout_t(std::string name): m_name{ name } {}

    auto descriptor_set_layout_t::binding_add
    (
        std::uint32_t binding,
        vk::DescriptorType type,
        std::uint32_t count,
        vk::ShaderStageFlagBits shader_visibility
    )
    -> void
    {
        vk::DescriptorSetLayoutBinding descriptor_layout_binding
        {
		    .binding = binding,
            .descriptorType = type,
            .descriptorCount = count,
            .stageFlags = shader_visibility
        };
        m_binding_arr.push_back(descriptor_layout_binding);
        return;
    }

    auto descriptor_set_layout_t::serialize(vk::raii::Device& device) -> void
    {
        vk::DescriptorSetLayoutCreateInfo descriptor_layout_info
        {
            .bindingCount{ static_cast<std::uint32_t>(m_binding_arr.size()) },
            .pBindings{ m_binding_arr.data() }
        };
        m_interface = vk::raii::DescriptorSetLayout{ device, descriptor_layout_info };
        vulkan_name(device, *m_interface, [&] { return m_name + ".descriptor-set-layout"; });
        return;
    }

    auto descriptor_set_layout_t::interface_get() -> vk::raii::DescriptorSetLayout&
    {
        return m_interface;
    }
}
