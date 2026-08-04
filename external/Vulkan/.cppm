module external.Vulkan;

import std;

export import vulkan;

export {
    using ::vk::ApplicationInfo;

    constexpr auto Vulkan_make_version(
        int ver_0,
        int ver_1,
        int ver_2,
        int ver_3
    ) -> std::uint32_t {
        return VK_MAKE_API_VERSION(ver_0, ver_1, ver_2, ver_3);
    }
}
