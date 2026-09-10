export module external.Vulkan;

import std;

export import vulkan;

export
{
    constexpr auto Vulkan_version_encode(
        int ver_0,
        int ver_1,
        int ver_2,
        int ver_3
    ) -> std::uint32_t {
        return vk::makeApiVersion(ver_0, ver_1, ver_2, ver_3);
    }
}
