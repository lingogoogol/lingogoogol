export module lgo.GUI.Vulkan:image_view;

import std;

import external.Vulkan;

import :image;

export namespace lgo
{
    class image_view_t
    {
    public:
        struct ctor_t
        {
            vk::raii::Device* device{};
            vk::raii::Image* image{};
            vk::Format format{};
            vk::ImageAspectFlags aspect{};
        };

        image_view_t() = default;
        image_view_t(const ctor_t& ctor);
        auto init(const ctor_t& ctor) -> void;

        auto interface_get() -> vk::raii::ImageView&;
    private:
        vk::raii::ImageView m_interface{ nullptr };
    };
}
