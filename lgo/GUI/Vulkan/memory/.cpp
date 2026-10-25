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
        if (!ctor.device || !ctor.device_physical || !*(*ctor.device) || !*(*ctor.device_physical) ||
            bool(ctor.buffer) == bool(ctor.image))
            throw std::invalid_argument{ "Vulkan memory allocation requires a device and exactly one buffer or image." };
        vk::MemoryRequirements memory_requirement{};
        if (ctor.buffer)
        {
            memory_requirement = ctor.buffer->getMemoryRequirements();
        }
        else if (ctor.image)
        {
            memory_requirement = ctor.image->getMemoryRequirements();
        }
        const auto memory_type_index{ vulkan_memory_type_index(memory_requirement,
            ctor.device_physical->getMemoryProperties(), ctor.property) };
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
