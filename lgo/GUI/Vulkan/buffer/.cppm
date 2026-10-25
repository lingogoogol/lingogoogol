export module lgo.GUI.Vulkan:buffer;

import std;

import external.Vulkan;

import lgo.GUI.texture;

import :memory;

export namespace lgo
{
    class buffer_t
    {
    public:
        struct ctor_t
        {
            vk::raii::PhysicalDevice* device_physical{};
            vk::raii::Device* device{};
            vk::BufferUsageFlagBits usage{};
            vk::MemoryPropertyFlags property{};
            vk::DeviceSize size{};
            const pixmap_t* pixmap{};
        };

        buffer_t() = default;
        buffer_t(buffer_t&&) = default;
        auto operator=(buffer_t&& other) noexcept -> buffer_t&;
        auto clear() noexcept -> void;
        buffer_t(const ctor_t& ctor);
        auto init(const ctor_t& ctor) -> void;

        auto interface_get() -> vk::raii::Buffer&;
        auto memory_get() -> memory_t&;
    private:
        memory_t m_memory{};
        vk::raii::Buffer m_interface{ nullptr };
    };
}
