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

    inline auto vulkan_surface_format(std::span<const vk::SurfaceFormatKHR> formats) -> vk::SurfaceFormatKHR
    {
        if (formats.empty()) throw vulkan_unsupported_t{ "The Vulkan surface exposes no image formats." };
        // VK_FORMAT_UNDEFINED as the sole entry means the application chooses.
        if (formats.size() == 1 && formats.front().format == vk::Format::eUndefined &&
            formats.front().colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            return { vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear };
        for (const auto preferred : { vk::Format::eB8G8R8A8Srgb, vk::Format::eR8G8B8A8Srgb,
            vk::Format::eB8G8R8A8Unorm, vk::Format::eR8G8B8A8Unorm })
            for (const auto& format : formats)
                if (format.format == preferred && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
                    return format;
        throw vulkan_unsupported_t{ "The Vulkan surface has no supported SDR RGBA/BGRA image format." };
    }

    inline auto vulkan_composite_alpha(vk::CompositeAlphaFlagsKHR supported) -> vk::CompositeAlphaFlagBitsKHR
    {
        for (const auto alpha : { vk::CompositeAlphaFlagBitsKHR::eOpaque, vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
            vk::CompositeAlphaFlagBitsKHR::ePostMultiplied, vk::CompositeAlphaFlagBitsKHR::eInherit })
            if (supported & alpha) return alpha;
        throw vulkan_unsupported_t{ "The Vulkan surface exposes no supported composite alpha mode." };
    }
}
