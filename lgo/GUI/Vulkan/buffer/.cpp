module lgo.GUI.Vulkan;

import std;

import external.Vulkan;

import lgo.GUI.texture;

namespace lgo
{
    buffer_t::buffer_t(const ctor_t& ctor)
    {
        init(ctor);
        return;
    }

    auto buffer_t::init(const ctor_t& ctor) -> void
    {
        ctor_t ctor_copy{ ctor };
        if (ctor_copy.pixmap)
        {
            ctor_copy.size = 4 * ctor_copy.pixmap->dim_get().x * ctor_copy.pixmap->dim_get().y;
            ctor_copy.usage = vk::BufferUsageFlagBits::eTransferSrc;
            ctor_copy.property = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
        }
        vk::BufferCreateInfo vertex_buffer_info
        {
            .size{ ctor_copy.size },
            .usage{ ctor_copy.usage },
            .sharingMode{ vk::SharingMode::eExclusive }
        };
        m_interface = vk::raii::Buffer{ *ctor_copy.device, vertex_buffer_info };
        m_memory.init
        ({
            .device_physical{ ctor_copy.device_physical },
            .device{ ctor_copy.device },
            .buffer{ &m_interface },
            .property{ ctor_copy.property }
        });
        m_interface.bindMemory(*m_memory.interface_get(), 0);
        if (ctor_copy.pixmap)
        {
            auto dest_ptr
            {
                reinterpret_cast<std::byte*>(m_memory.interface_get().mapMemory(0, ctor_copy.size))
            };
            for (std::uint64_t y{ 0 }; y < static_cast<std::uint64_t>(ctor_copy.pixmap->dim_get().y); ++y)
            {
                for (std::uint64_t x{ 0 }; x < static_cast<std::uint64_t>(ctor_copy.pixmap->dim_get().x); ++x)
                {
                    std::uint64_t offset{ ctor_copy.pixmap->dim_get().x * y + x };
                    dest_ptr[offset] = (*ctor_copy.pixmap)[x, y];
                }
            }
            m_memory.interface_get().unmapMemory();
        }
        return;
    }

    auto buffer_t::interface_get() -> vk::raii::Buffer&
    {
        return m_interface;
    }

    auto buffer_t::memory_get() -> memory_t&
    {
        return m_memory;
    }
}
