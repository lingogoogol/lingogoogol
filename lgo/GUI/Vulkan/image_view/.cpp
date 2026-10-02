module lgo.GUI.Vulkan;

import std;

import external.Vulkan;

namespace lgo
{
    image_view_t::image_view_t(const ctor_t& ctor)
    {
        vk::ImageViewCreateInfo info
        {
            .image{ *ctor.image },
            .viewType{ vk::ImageViewType::e2D },
            .format{ ctor.format },
            .subresourceRange
            {
                .aspectMask{ vk::ImageAspectFlagBits::eColor },
                .baseMipLevel{ 0 },
                .levelCount{ 1 },
                .baseArrayLayer{ 0 },
                .layerCount{ 1 }
            }
        };
        m_interface = vk::raii::ImageView{ *ctor.device, info };
        return;
    }

    auto image_view_t::interface_get() -> vk::ImageView&
    {
        return m_interface;
    }
}
