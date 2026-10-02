module lgo.GUI.Vulkan;

import std;

import external.Vulkan;

import lgo.GUI.texture;

namespace lgo
{
    image_t::image_t(const ctor_t& ctor)
    {
        init(ctor);
        return;
    }

    auto image_t::init(const ctor_t& ctor) -> void
    {
        ctor_t ctor_copy{ ctor };
        if (m_pixmap)
        {
            m_pixmap = ctor_copy.pixmap;
            m_buffer.init
            ({
                .device_physical{ ctor_copy.device_physical },
                .device{ ctor_copy.device },
                .pixmap{ ctor_copy.pixmap }
            });
            ctor_copy.width = static_cast<std::uint32_t>(ctor_copy.pixmap->dim_get().x);
            ctor_copy.height = static_cast<std::uint32_t>(ctor_copy.pixmap->dim_get().y);
            ctor_copy.format = vk::Format::eR8Srgb;
            ctor_copy.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
            ctor_copy.property = vk::MemoryPropertyFlagBits::eDeviceLocal;
        }

        m_name = ctor_copy.name;
        vk::ImageCreateInfo image_info
        {
            .imageType{ vk::ImageType::e2D },
            .format{ ctor_copy.format },
            .extent{ ctor_copy.width, ctor_copy.height, 1 },
            .mipLevels{ 1 },
            .arrayLayers{ 1 },
            .samples{ vk::SampleCountFlagBits::e1 },
            .tiling{ vk::ImageTiling::eOptimal },
            .usage{ ctor_copy.usage },
            .sharingMode{ vk::SharingMode::eExclusive }
        };
        m_interface = vk::raii::Image{ *ctor_copy.device, image_info };

        m_memory.init
        ({
            .device_physical{ ctor_copy.device_physical },
            .device{ ctor_copy.device },
            .image{ &m_interface }
        });
        m_interface.bindMemory(*m_memory.interface_get(), 0);

        barrier
        (
            *ctor_copy.command_buffer,
            m_interface,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal,
            {},
            vk::AccessFlagBits::eTransferWrite,
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eTransfer
        );
        vk::BufferImageCopy region
        {
            .bufferOffset{ 0 },
            .bufferRowLength{ 0 },
            .bufferImageHeight{ 0 },
            .imageSubresource
            {
                .aspectMask{ vk::ImageAspectFlagBits::eColor },
                .mipLevel{ 0 },
                .baseArrayLayer{ 0 },
                .layerCount{ 1 }
            },
            .imageOffset{ 0, 0, 0 },
            .imageExtent{ ctor_copy.width, ctor_copy.height, 1 }
        };
        ctor_copy.command_buffer->copyBufferToImage
        (
            m_buffer.interface_get(),
            m_interface,
            vk::ImageLayout::eTransferDstOptimal,
            region
        );
        barrier
        (
            *ctor_copy.command_buffer,
            m_interface,
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::AccessFlagBits::eTransferWrite,
            vk::AccessFlagBits::eShaderRead,
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eFragmentShader
        );

        return;
    }

    auto image_t::interface_get() -> vk::raii::Image&
    {
        return m_interface;
    }

    auto image_t::barrier
    (
        vk::raii::CommandBuffer& command_buffer,
        vk::raii::Image& image,
        vk::ImageLayout layout_old,
        vk::ImageLayout layout_new,
        vk::AccessFlags access_src,
        vk::AccessFlags access_dest,
        vk::PipelineStageFlags stage_src,
        vk::PipelineStageFlags stage_dest
    )
    -> void
    {
        vk::ImageMemoryBarrier barrier_v
        {
            .srcAccessMask{ access_src },
            .dstAccessMask{ access_dest },
            .oldLayout{ layout_old },
            .newLayout{ layout_new },
            .srcQueueFamilyIndex{ vk::QueueFamilyIgnored },
            .dstQueueFamilyIndex{ vk::QueueFamilyIgnored },
            .image{ image },
            .subresourceRange
            {
                .aspectMask{ vk::ImageAspectFlagBits::eColor },
                .levelCount{ 1 },
                .layerCount{ 1 }
            }
        };
        command_buffer.pipelineBarrier(stage_src, stage_dest, {}, {}, {}, barrier_v);
        return;
    }
}
