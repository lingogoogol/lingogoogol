module lgo.GUI.Vulkan;

import std;

import external.Vulkan;

import lgo.io.file;

namespace lgo
{
    shader_module_t::shader_module_t(vk::raii::Device& device, const std::string& path)
    {
        const auto length{ std::filesystem::file_size(path) };
        if (length < 5 * sizeof(std::uint32_t) || length % sizeof(std::uint32_t))
            throw std::runtime_error{ "Invalid SPIR-V shader length: " + path };
        std::vector<unsigned char> shader_code{ get_file(path) };
        // Copy into word storage: the SPIR-V API requires uint32_t alignment.
        std::vector<std::uint32_t> words(shader_code.size() / sizeof(std::uint32_t));
        std::memcpy(words.data(), shader_code.data(), shader_code.size());
        if (words.front() != 0x07230203)
            throw std::runtime_error{ "Invalid SPIR-V shader magic: " + path };
        vk::ShaderModuleCreateInfo shader_module_info
        {
            .codeSize{ shader_code.size() },
            .pCode{ words.data() }
        };
        m_interface = vk::raii::ShaderModule{ device, shader_module_info };
        vulkan_name(device, *m_interface, [&] { return path + ".shader-module"; });
        return;
    }

    auto shader_module_t::interface_get() -> vk::raii::ShaderModule& {
        return m_interface;
    }
}
