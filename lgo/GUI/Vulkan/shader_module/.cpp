module lgo.GUI.Vulkan;

import std;

import external.Vulkan;

import lgo.io.file;

namespace lgo
{
    shader_module_t::shader_module_t(vk::raii::Device& device, const std::string& path)
    {
        std::vector<unsigned char> shader_code{ get_file(path) };
        //The default allocator of std::vector already ensures that the data satisfies the alignment requirements of uint32_t.
        vk::ShaderModuleCreateInfo shader_module_info
        {
            .codeSize{ shader_code.size() },
            .pCode{ reinterpret_cast<const std::uint32_t*>(shader_code.data()) }
        };
        m_interface = vk::raii::ShaderModule{ device, shader_module_info };
        vulkan_name(device, *m_interface, [&] { return path + ".shader-module"; });
        return;
    }

    auto shader_module_t::interface_get() -> vk::raii::ShaderModule& {
        return m_interface;
    }
}
