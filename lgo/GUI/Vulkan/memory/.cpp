module lgo.GUI.Vulkan;

import std;

import external.Vulkan;

namespace lgo
{
    memory_t::memory_t(const ctor_t& ctor)
    {
        init(ctor);
        return;
    }

    auto memory_t::init(const ctor_t& ctor) -> void
    {
        vk::MemoryRequirements memory_requirement{};
        if (ctor.buffer)
        {
            memory_requirement = ctor.buffer->getMemoryRequirements();
        }
        else if (ctor.image)
        {
            memory_requirement = ctor.image->getMemoryRequirements();
        }
        else
        {
            throw;
        }
        vk::PhysicalDeviceMemoryProperties memory_property{ ctor.device_physical->getMemoryProperties() };
        auto memory_type_index{ std::numeric_limits<std::uint32_t>::max() };
        for (std::uint32_t i{ 0 }; i < memory_property.memoryTypeCount; ++i)
        {
            if
            (
                (memory_requirement.memoryTypeBits >> i) &&
                !(~memory_property.memoryTypes[i].propertyFlags & ctor.property)
            )
            {
                memory_type_index = i;
                break;
            }
        }
        if (memory_type_index == std::numeric_limits<std::uint32_t>::max())
        {
            throw;
        }
        vk::MemoryAllocateInfo memory_info
        {
            .allocationSize  = memory_requirement.size,
            .memoryTypeIndex = memory_type_index
        };
        m_interface = vk::raii::DeviceMemory{ *ctor.device, memory_info };
        vulkan_name(*ctor.device, *m_interface, [&]
        {
            return std::string{ ctor.name } + (ctor.buffer ? ".buffer.memory" : ".image.memory");
        });
        return;
    }

    auto memory_t::interface_get() -> vk::raii::DeviceMemory&
    {
        return m_interface;
    }
}
