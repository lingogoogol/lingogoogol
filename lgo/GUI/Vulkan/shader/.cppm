export module lgo.GUI.Vulkan:shader;

import std;

import external.Vulkan;

import :shader_module;

export namespace lgo
{
    class shader_t
    {
    public:
        shader_t(vk::ShaderStageFlagBits stage, shader_module_t* module, std::string name);

        auto stage_get() const -> vk::ShaderStageFlagBits;
        auto module_get() -> shader_module_t&;
        auto name_get() const -> const std::string&;
    private:
        vk::ShaderStageFlagBits m_stage{};
        shader_module_t* m_module{};
        std::string m_name{};
    };
}
