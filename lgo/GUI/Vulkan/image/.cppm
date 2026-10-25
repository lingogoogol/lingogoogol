export module lgo.GUI.Vulkan:image;

import std;

import external.Vulkan;

import lgo.GUI.texture;

import :buffer;
import :memory;

export namespace lgo
{
    class image_t
    {
    public:
        struct ctor_t
        {
            std::string name{};
            vk::raii::PhysicalDevice* device_physical{};
            vk::raii::Device* device{};
            vk::raii::CommandBuffer* command_buffer{};
            buffer_t* buffer{};
            std::uint32_t width{};
            std::uint32_t height{};
            const pixmap_t* pixmap{};
            vk::Format format{};
            vk::ImageUsageFlags usage{};
            vk::MemoryPropertyFlags property{};
        };

        image_t() = default;
        image_t(image_t&&) = default;
        auto operator=(image_t&& other) noexcept -> image_t&;
        auto clear() noexcept -> void;
        image_t(const ctor_t& ctor);
        auto init(const ctor_t& ctor) -> void;

        auto interface_get() -> vk::raii::Image&;
        auto barrier
        (
            vk::raii::CommandBuffer& command_buffer,
            vk::ImageAspectFlags aspect,
            vk::ImageLayout layout_old,
            vk::ImageLayout layout_new,
            vk::AccessFlags access_src,
            vk::AccessFlags access_dest,
            vk::PipelineStageFlags stage_src,
            vk::PipelineStageFlags stage_dest
        )
        -> void;
    private:
        std::string m_name{};
        const pixmap_t* m_pixmap{};
        memory_t m_memory{};
        buffer_t m_buffer{};
        vk::raii::Image m_interface{ nullptr };
    };
}
