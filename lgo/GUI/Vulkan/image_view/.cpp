module lgo.GUI.Vulkan;

import std;

import external.Vulkan;

namespace lgo
{
    image_view_t::image_view_t(const ctor_t& ctor)
    {
        init(ctor);
        return;
    }
    
    auto image_view_t::init(const ctor_t& ctor) -> void
    {
        vk::ImageViewCreateInfo info
        {
            .image{ *ctor.image },
            .viewType{ vk::ImageViewType::e2D },
            .format{ ctor.format },
            .subresourceRange
            {
                .aspectMask{ ctor.aspect },
                .baseMipLevel{ 0 },
                .levelCount{ 1 },
                .baseArrayLayer{ 0 },
                .layerCount{ 1 }
            }
        };
        // Glyphs store coverage in R8; swizzle them to white RGBA so the same
        // texture shader can render both colored photographs and tinted text.
        if (ctor.format == vk::Format::eR8Unorm)
            info.components = {vk::ComponentSwizzle::eOne,vk::ComponentSwizzle::eOne,
                               vk::ComponentSwizzle::eOne,vk::ComponentSwizzle::eR};
        m_interface = vk::raii::ImageView{ *ctor.device, info };
        vulkan_name(*ctor.device, *m_interface, [&] { return std::string{ ctor.name } + ".image-view"; });
        return;
    }

    auto image_view_t::interface_get() -> vk::raii::ImageView&
    {
        return m_interface;
    }
}
