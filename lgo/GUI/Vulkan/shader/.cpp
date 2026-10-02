module lgo.GUI.Vulkan;

import std;

import external.Vulkan;

namespace lgo
{
    shader_t::shader_t(vk::ShaderStageFlagBits stage, shader_module_t* shader_module, std::string name)
    : m_stage{ stage }, m_module{ std::move(shader_module) }, m_name{ name } {}

    auto shader_t::stage_get() const -> vk::ShaderStageFlagBits
    {
        return m_stage;
    }

    auto shader_t::module_get() -> shader_module_t&
    {
        return *m_module;
    }

    auto shader_t::name_get() const -> std::string
    {
        return m_name;
    }
}
