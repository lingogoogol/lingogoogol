module lgo.GUI.Vulkan;

import std;

import external.Vulkan;

import lgo.GUI.texture;

namespace lgo
{
    auto buffer_t::clear() noexcept -> void
    {
        // Bound objects must be destroyed before their backing memory.
        m_interface.clear();
        m_memory = {};
    }

    auto buffer_t::operator=(buffer_t&& other) noexcept -> buffer_t&
    {
        if (this != &other)
        {
            clear();
            m_memory = std::move(other.m_memory);
            m_interface = std::move(other.m_interface);
        }
        return *this;
    }

    buffer_t::buffer_t(const ctor_t& ctor)
    {
        init(ctor);
        return;
    }

    auto buffer_t::init(const ctor_t& ctor) -> void
    {
        clear();
        if (!ctor.device || !ctor.device_physical || !*(*ctor.device) || !*(*ctor.device_physical))
            throw std::invalid_argument{ "Vulkan buffer creation requires a physical and logical device." };
        ctor_t ctor_copy{ ctor };
        if (ctor_copy.pixmap)
        {
            const auto dimensions{ ctor_copy.pixmap->dim_get() };
            if (dimensions.x <= 0 || dimensions.y <= 0 || static_cast<std::uint64_t>(dimensions.x) >
                std::numeric_limits<std::uint64_t>::max() / static_cast<std::uint64_t>(dimensions.y))
                throw std::invalid_argument{ "Vulkan staging buffer dimensions must be positive and fit in 64 bits." };
            ctor_copy.size = static_cast<std::uint64_t>(dimensions.x) * static_cast<std::uint64_t>(dimensions.y);
            ctor_copy.usage = vk::BufferUsageFlagBits::eTransferSrc;
            ctor_copy.property = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
        }
        if (!ctor_copy.size || !std::to_underlying(ctor_copy.usage))
            throw std::invalid_argument{ "Vulkan buffers require a nonzero size and usage." };
        vk::BufferCreateInfo vertex_buffer_info
        {
            .size{ ctor_copy.size },
            .usage{ ctor_copy.usage },
            .sharingMode{ vk::SharingMode::eExclusive }
        };
        m_interface = vk::raii::Buffer{ *ctor_copy.device, vertex_buffer_info };
        vulkan_name(*ctor_copy.device, *m_interface, [&]
        {
            return std::string{ ctor_copy.name } + (ctor_copy.pixmap ? ".staging.buffer" : ".buffer");
        });
        m_memory.init
        ({
            .name{ ctor_copy.name },
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
