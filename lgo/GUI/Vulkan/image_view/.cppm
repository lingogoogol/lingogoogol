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
        };
        image_view_t() = default;
        image_view_t(const ctor_t& ctor);

        auto interface_get() -> vk::ImageView&;
    private:
        vk::ImageView m_interface{ nullptr };
    };
}
