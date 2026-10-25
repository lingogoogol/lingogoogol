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
        m_interface = vk::raii::ImageView{ *ctor.device, info };
        vulkan_name(*ctor.device, *m_interface, [&] { return std::string{ ctor.name } + ".image-view"; });
        return;
    }

    auto image_view_t::interface_get() -> vk::raii::ImageView&
    {
        return m_interface;
    }
}
