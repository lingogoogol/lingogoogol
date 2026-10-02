export module lgo.GUI.Vulkan:memory;

import external.Vulkan;

export namespace lgo
{
    class memory_t
    {
    public:
        struct ctor_t
        {
            vk::raii::PhysicalDevice* device_physical{};
            vk::raii::Device* device{};
            vk::raii::Buffer* buffer{};
            vk::raii::Image* image{};
            vk::MemoryPropertyFlags property{};
        };

        memory_t() = default;
        memory_t(const ctor_t& ctor);
        auto init(const ctor_t& ctor) -> void;

        auto interface_get() -> vk::raii::DeviceMemory&;
    private:
        vk::raii::DeviceMemory m_interface{ nullptr };
    };
}
