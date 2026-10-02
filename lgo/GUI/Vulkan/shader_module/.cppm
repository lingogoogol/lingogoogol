export module lgo.GUI.Vulkan:shader_module;

import std;

import external.Vulkan;

export namespace lgo
{
    class shader_module_t
    {
    public:
        shader_module_t() = default;
        shader_module_t(vk::raii::Device& device, const std::string& path);

        auto interface_get() -> vk::raii::ShaderModule&;
    private:
        vk::raii::ShaderModule m_interface{ nullptr };
    };
}
