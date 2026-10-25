export module lgo.GUI.Vulkan:support;

import std;
import external.Vulkan;

export namespace lgo
{
    class vulkan_unsupported_t: public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    inline auto vulkan_memory_type_index(const vk::MemoryRequirements& requirements,
        const vk::PhysicalDeviceMemoryProperties& properties, vk::MemoryPropertyFlags requested) -> std::uint32_t
    {
        for (std::uint32_t i{}; i < properties.memoryTypeCount; ++i)
        {
            // memoryTypeBits is a bit set, not a lower bound on valid indices.
            // Avoid vk::Flags' defaulted operator==: Clang 23 crashes emitting it
            // across C++ module partitions. Test for missing bits instead.
            if ((requirements.memoryTypeBits & (std::uint32_t{ 1 } << i)) &&
                !(~properties.memoryTypes[i].propertyFlags & requested))
                return i;
        }
        throw vulkan_unsupported_t{ std::format(
            "No compatible Vulkan memory type (type bits {:#x}, required properties {:#x}).",
            requirements.memoryTypeBits, static_cast<std::uint32_t>(requested)) };
    }

}
