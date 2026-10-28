module;

#include <glm/glm.hpp>

module lgo.GUI.primitive;

import std;

import external.Vulkan;

import lgo.io.file;
import lgo.math.vec;
import lgo.math.op;
import lgo.GUI.misc;
import lgo.GUI.Vulkan;

namespace lgo
{
    engine_t* rect_primitive_t::m_engine{};
    descriptor_set_layout_t rect_primitive_t::m_solid_descriptor_set_layout{ "rect.solid" };
    descriptor_set_layout_t rect_primitive_t::m_texture_descriptor_set_layout{ "rect.texture" };
    pipeline_state_t rect_primitive_t::m_solid_pipeline_state{};
    pipeline_state_t rect_primitive_t::m_texture_pipeline_state{};
    vk::raii::Sampler rect_primitive_t::m_sampler{ nullptr };
    buffer_t rect_primitive_t::m_cursor_vertices{};

    auto rect_primitive_t::create_vertex_buffer() -> void
    {
        if (m_texture_enable)
        {
            m_vertex_buffer_size = sizeof(texture_vertex_t) * vertex_data_len;
        }
        else
        {
            m_vertex_buffer_size = sizeof(solid_vertex_t) * vertex_data_len;
        }
        
        m_vertex_buffer.init
        ({
            .name{ m_name.empty() ? std::string_view{ "rect.vertex" } : std::string_view{ m_name } },
            .device_physical{ &m_engine->device_physical_get() },
            .device{ &m_engine->device_get() },
            .usage{ vk::BufferUsageFlagBits::eVertexBuffer },
            .property{ vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent },
            .size{ m_vertex_buffer_size }
        });
        return;
    }

    auto rect_primitive_t::upload_vertex_data() -> void
    try
    {
        auto gpu_guard{ m_engine->gpu_lock() };
        if (!m_engine->gpu_available()) return;
        std::unique_lock lock{ m_mutex };
        float pos_x{ static_cast<float>(m_pos.x) / static_cast<float>(m_engine->get_window_size().x) * 2.0f - 1.0f };
        float pos_y{ static_cast<float>(m_pos.y) / static_cast<float>(m_engine->get_window_size().y) * 2.0f - 1.0f };
        float size_x{ static_cast<float>(m_size.x) / static_cast<float>(m_engine->get_window_size().x) * 2.0f };
        float size_y{ static_cast<float>(m_size.y) / static_cast<float>(m_engine->get_window_size().y) * 2.0f };

        //texture_index is not used.
        std::uint32_t texture_index{ 0 };
        if (m_texture_enable)
        {
            //Compute UV coordinates for any screen-space point `p` using `(m_texture_pos, m_texture_axis_x, m_texture_axis_y)`
            //as the 2D affine basis of the texture's [0, 1] x [0, 1] UV space:
            //  p = m_texture_pos + u * m_texture_axis_x + v * m_texture_axis_y.
            //If `det == 0` (e.g. a whitespace glyph with a 0x0 bounding box), return `(-1, -1)` so the
            //border-clamped sampler evaluates to transparent black `(0, 0, 0, 0)` instead of dividing by zero.
            std::int64_t det
            {
                m_texture_axis_x.x * m_texture_axis_y.y - m_texture_axis_y.x * m_texture_axis_x.y
            };
            auto calc_uv
            {
                [this, det] (pos_2D p) -> glm::vec2
                {
                    if (det == 0)
                    {
                        return { -1.0f, -1.0f };
                    }
                    size_2D d{ p - m_texture_pos };
                    float det_f{ static_cast<float>(det) };
                    return
                    {
                        static_cast<float>(d.x * m_texture_axis_y.y - d.y * m_texture_axis_y.x) / det_f,
                        static_cast<float>(-d.x * m_texture_axis_x.y + d.y * m_texture_axis_x.x) / det_f
                    };
                }
            };
            glm::vec3 color{ m_color.R, m_color.G, m_color.B };
            auto vertex_data
            {
                std::to_array<texture_vertex_t>
                ({
                    {
                        { pos_x, pos_y, m_depth },
                        texture_index,
                        calc_uv(m_pos),
                        color
                    },
                    {
                        { pos_x + size_x, pos_y, m_depth },
                        texture_index,
                        calc_uv(m_pos + size_2D{ m_size.x, 0 }),
                        color
                    },
                    {
                        { pos_x, pos_y + size_y, m_depth },
                        texture_index,
                        calc_uv(m_pos + size_2D{ 0, m_size.y }),
                        color
                    },
                    {
                        { pos_x + size_x, pos_y + size_y, m_depth },
                        texture_index,
                        calc_uv(m_pos + m_size),
                        color
                    }
                })
            };
            upload_vertex_data(vertex_data);
        }
        else
        {
            auto vertex_data
            {
                std::to_array<solid_vertex_t>
                ({
                    { { pos_x, pos_y, m_depth }, { m_color.R, m_color.G, m_color.B } },
                    { { pos_x + size_x, pos_y, m_depth }, { m_color.R, m_color.G, m_color.B } },
                    { { pos_x, pos_y + size_y, m_depth }, { m_color.R, m_color.G, m_color.B } },
                    { { pos_x + size_x, pos_y + size_y, m_depth }, { m_color.R, m_color.G, m_color.B } }
                })
            };
            upload_vertex_data(vertex_data);
        }
        return;
    }

    catch (const vk::DeviceLostError&)
    {
        m_engine->device_lost();
    }

    template<typename t_type>
    auto rect_primitive_t::upload_vertex_data(const t_type& vertex_data) -> void
    {
        auto vertex_ptr{ reinterpret_cast<const std::byte*>(vertex_data.data()) };
        auto vertex_buffer_ptr
        {
            reinterpret_cast<std::byte*>
            (
                m_vertex_buffer.memory_get().interface_get().mapMemory
                (
                    0, 
                    m_vertex_buffer_size
                )
            )
        };
        for (int i{ 0 }; i < m_vertex_buffer_size; ++i)
        {
            vertex_buffer_ptr[i] = vertex_ptr[i];
        }
        m_vertex_buffer.memory_get().interface_get().unmapMemory();
        return;
    }

    auto rect_primitive_t::init(engine_t* engine) -> void
    {
        m_engine = engine;

        m_solid_descriptor_set_layout = descriptor_set_layout_t{ "rect.solid" };
        m_texture_descriptor_set_layout = descriptor_set_layout_t{ "rect.texture" };
        m_solid_descriptor_set_layout.serialize(m_engine->device_get());
        m_solid_pipeline_state.init
        ({
            .name{ "rect_primitive_t::solid_pipeline_state" },
            .descriptor_set_layout{ &m_solid_descriptor_set_layout },
            .topology{ vk::PrimitiveTopology::eTriangleStrip },
            .surface_format{ m_engine->format_get() },
            .depth_format{ m_engine->depth_format_get() },
            .push_constant_size{32}
        });
        m_solid_pipeline_state.vertex_binding_add
        (
            0,
            sizeof(solid_vertex_t),
            vk::VertexInputRate::eVertex
        );
        m_solid_pipeline_state.vertex_attribute_add
        (
            0,
            0,
            vk::Format::eR32G32B32Sfloat,
            0
        );
        m_solid_pipeline_state.vertex_attribute_add
        (
            1,
            0, 
            vk::Format::eR32G32B32Sfloat,
            16
        );
        shader_module_t solid_shader_module{ engine->device_get(), "shader/solid.spv" };
        shader_t solid_shader_vertex
        {
            vk::ShaderStageFlagBits::eVertex,
            &solid_shader_module,
            "vertex_main"
        };
        m_solid_pipeline_state.shader_add(solid_shader_vertex);
        shader_t solid_shader_pixel
        {
            vk::ShaderStageFlagBits::eFragment,
            &solid_shader_module,
            "fragment_main"
        };
        m_solid_pipeline_state.shader_add(solid_shader_pixel);
        m_solid_pipeline_state.serialize(m_engine->device_get());
        // One immutable quad; cursor movement changes push constants only,
        // avoiding vertex uploads and device-idle waits on every mouse event.
        m_cursor_vertices.init({.name{"cursor.highlight.quad"},
            .device_physical{&engine->device_physical_get()}, .device{&engine->device_get()},
            .usage{vk::BufferUsageFlagBits::eVertexBuffer},
            .property{vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent},
            .size{sizeof(solid_vertex_t) * 4}});
        auto* data{m_cursor_vertices.memory_get().interface_get().mapMemory(0, sizeof(solid_vertex_t) * 4)};
        std::memset(data, 0, sizeof(solid_vertex_t) * 4);
        m_cursor_vertices.memory_get().interface_get().unmapMemory();

        m_texture_descriptor_set_layout.binding_add
        (
            0,
            vk::DescriptorType::eCombinedImageSampler,
            1,
            vk::ShaderStageFlagBits::eFragment
        );
        m_texture_descriptor_set_layout.serialize(m_engine->device_get());
        m_texture_pipeline_state.init
        ({
            .name{ "rect_primitive_t::texture_pipeline_state" },
            .descriptor_set_layout{ &m_texture_descriptor_set_layout },
            .topology{ vk::PrimitiveTopology::eTriangleStrip },
            .surface_format{ m_engine->format_get() },
            .depth_format{ m_engine->depth_format_get() }
        });
        m_texture_pipeline_state.vertex_binding_add
        (
            0,
            sizeof(texture_vertex_t),
            vk::VertexInputRate::eVertex
        );
        m_texture_pipeline_state.vertex_attribute_add
        (
            0,
            0,
            vk::Format::eR32G32B32Sfloat,
            0
        );
        m_texture_pipeline_state.vertex_attribute_add
        (
            1,
            0,
            vk::Format::eR32Uint,
            12
        );
        m_texture_pipeline_state.vertex_attribute_add
        (
            2,
            0,
            vk::Format::eR32G32Sfloat,
            16
        );
        m_texture_pipeline_state.vertex_attribute_add
        (
            3,
            0,
            vk::Format::eR32G32B32Sfloat,
            32
        );
        shader_module_t texture_shader_module{ engine->device_get(), "shader/texture.spv" };
        shader_t texture_shader_vertex
        {
            vk::ShaderStageFlagBits::eVertex,
            &texture_shader_module,
            "vertex_main"
        };
        m_texture_pipeline_state.shader_add(texture_shader_vertex);
        shader_t texture_shader_pixel
        {
            vk::ShaderStageFlagBits::eFragment,
            &texture_shader_module,
            "fragment_main"
        };
        m_texture_pipeline_state.shader_add(texture_shader_pixel);
        m_texture_pipeline_state.serialize(engine->device_get());

        vk::PhysicalDeviceProperties device_physical_property{ engine->device_physical_get().getProperties() };
        //Use `eClampToBorder` with `eFloatTransparentBlack` so that fragments outside `[0, 1] x [0, 1]`
        //(where the rectangle cell `m_pos..m_pos+m_size` extends beyond the glyph's `m_texture_pos` bounding box)
        //sample as transparent `(0, 0, 0, 0)` instead of repeating or smearing edge texels.
        vk::SamplerCreateInfo sampler_info
        {
            .magFilter{ vk::Filter::eLinear },
            .minFilter{ vk::Filter::eLinear },
            .mipmapMode{ vk::SamplerMipmapMode::eLinear },
            .addressModeU{ vk::SamplerAddressMode::eClampToBorder },
            .addressModeV{ vk::SamplerAddressMode::eClampToBorder },
            .addressModeW{ vk::SamplerAddressMode::eClampToBorder },
            .mipLodBias{ 0.0f },
            .anisotropyEnable{ vk::False },
            .maxAnisotropy{ device_physical_property.limits.maxSamplerAnisotropy },
            .compareEnable{ vk::False },
            .compareOp{ vk::CompareOp::eAlways },
            .borderColor{ vk::BorderColor::eFloatTransparentBlack },
            .unnormalizedCoordinates{ vk::False }
        };
        m_sampler = vk::raii::Sampler{ engine->device_get(), sampler_info };
        vulkan_name(engine->device_get(), *m_sampler, "rect.glyph.sampler");

        return;
    }

    auto rect_primitive_t::uninit() -> void
    {
        m_solid_descriptor_set_layout = {};
        m_texture_descriptor_set_layout = {};
        m_solid_pipeline_state = {};
        m_texture_pipeline_state = {};
        m_sampler.clear();
        m_cursor_vertices.clear();
        return;
    }

    auto rect_primitive_t::render
    (
        const std::set<rect_primitive_t*>& rect,
        vk::raii::CommandBuffer& command_buffer, bool cursor_highlight
    )
    -> void
    {
        command_buffer.bindPipeline
        (
            vk::PipelineBindPoint::eGraphics,
            *m_solid_pipeline_state.interface_get()
        );
        const std::array<float, 8> ordinary{};
        command_buffer.pushConstants<float>(m_solid_pipeline_state.layout_get(),
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, ordinary);
        for (auto i{ rect.begin() }; i != rect.end(); ++i)
        {
            if (!(*i)->m_texture_enable)
            {
                (*i)->render(command_buffer);
            }
        }
        
        command_buffer.bindPipeline
        (
            vk::PipelineBindPoint::eGraphics,
            *m_texture_pipeline_state.interface_get()
        );
        for (auto i{ rect.begin() }; i != rect.end(); ++i)
        {
            if ((*i)->m_texture_enable)
            {
                (*i)->render(command_buffer);
            }
        }
        if (cursor_highlight)
        {
            const auto logical{m_engine->get_window_size()};
            const auto frame{m_engine->get_framebuffer_size()};
            const auto cursor{m_engine->cursor_pos_get()};
            const float sx{float(frame.x) / logical.x}, sy{float(frame.y) / logical.y};
            // Independent X/Y framebuffer scales preserve a logical circle on HiDPI screens.
            const std::array<float, 8> highlight{cursor.x * sx, cursor.y * sy,
                72.f * sx, 72.f * sy, float(frame.x), float(frame.y), 0.f, 0.f};
            command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_solid_pipeline_state.interface_get());
            command_buffer.pushConstants<float>(m_solid_pipeline_state.layout_get(),
                vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, highlight);
            command_buffer.setScissor(0, vk::Rect2D{.offset{0, 0},
                .extent{static_cast<std::uint32_t>(frame.x), static_cast<std::uint32_t>(frame.y)}});
            command_buffer.bindVertexBuffers(0, *m_cursor_vertices.interface_get(), {0});
            command_buffer.draw(4, 1, 0, 0);
        }
        return;
    }

    rect_primitive_t::rect_primitive_t(const ctor_t& ctor)
    :
        m_pos{ ctor.pos },
        m_size{ ctor.size },
        m_depth{ ctor.depth },
        m_clip_pos{ ctor.clip_pos },
        m_clip_size{ ctor.clip_size },
        m_color{ ctor.color },
        m_image_view{ ctor.image_view },
        m_texture_pos{ ctor.texture_pos },
        m_texture_axis_x{ ctor.texture_axis_x },
        m_texture_axis_y{ ctor.texture_axis_y },
        m_name{ ctor.name }
    {
        m_texture_enable = ctor.image_view;

        create_gpu();
    }

    auto rect_primitive_t::release_gpu() noexcept -> void
    {
        m_descriptor_set.clear();
        m_descriptor_pool.clear();
        m_vertex_buffer.clear();
    }

    auto rect_primitive_t::create_gpu() -> void
    try
    {
        auto gpu_guard{ m_engine->gpu_lock() };
        if (!m_engine->gpu_available()) return;
        create_vertex_buffer();
        upload_vertex_data();
        if (!m_engine->gpu_available()) return;

        if (m_texture_enable)
        {
            vk::DescriptorPoolSize descriptor_pool_size
            {
                .type{ vk::DescriptorType::eCombinedImageSampler },
                .descriptorCount{ 1 }
            };
            vk::DescriptorPoolCreateInfo descriptor_pool_info
            {
                .flags{ vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet },
                .maxSets{ 1 },
                .poolSizeCount{ 1 },
                .pPoolSizes = &descriptor_pool_size
            };
            m_descriptor_pool = vk::raii::DescriptorPool{ m_engine->device_get(), descriptor_pool_info };
            vulkan_name(m_engine->device_get(), *m_descriptor_pool, [&] { return m_name + ".descriptor-pool"; });

            std::vector<vk::DescriptorSetLayout> descriptor_set_layout( 1, *m_texture_descriptor_set_layout.interface_get() );
            vk::DescriptorSetAllocateInfo descriptor_set_info
            {
                .descriptorPool{ m_descriptor_pool },
                .descriptorSetCount{ static_cast<std::uint32_t>(descriptor_set_layout.size()) },
                .pSetLayouts{ descriptor_set_layout.data() }
            };
            m_descriptor_set = std::move(m_engine->device_get().allocateDescriptorSets(descriptor_set_info)[0]);
            vulkan_name(m_engine->device_get(), *m_descriptor_set, [&] { return m_name + ".descriptor-set"; });

            vk::DescriptorImageInfo descriptor_image_info
            {
                .sampler{ m_sampler },
                .imageView{ m_image_view->interface_get() },
                .imageLayout{ vk::ImageLayout::eShaderReadOnlyOptimal }
            };
            vk::WriteDescriptorSet descriptor_write
            {
                .dstSet{ m_descriptor_set },
                .dstBinding{ 0 },
                .dstArrayElement{ 0 },
                .descriptorCount{ 1 },
                .descriptorType{ vk::DescriptorType::eCombinedImageSampler },
                .pImageInfo{ &descriptor_image_info }
            };
            m_engine->device_get().updateDescriptorSets(descriptor_write, {});
        }

        return;
    }

    catch (const vk::DeviceLostError&)
    {
        m_engine->device_lost();
    }

    auto rect_primitive_t::set_pos(pos_2D pos) -> void
    {
        auto gpu_guard{ m_engine->gpu_lock() };
        std::unique_lock lock{ m_mutex };
        m_texture_pos = m_texture_pos + (pos - m_pos);
        m_pos = pos;
        m_engine->flush();
        upload_vertex_data();
        return;
    }

    auto rect_primitive_t::set_size(size_2D size) -> void
    {
        auto gpu_guard{ m_engine->gpu_lock() };
        std::unique_lock lock{ m_mutex };
        m_size = size;
        m_engine->flush();
        upload_vertex_data();
        return;
    }

    auto rect_primitive_t::set_color(color_t color) -> void
    {
        auto gpu_guard{ m_engine->gpu_lock() };
        std::unique_lock lock{ m_mutex };
        m_color = color;
        m_engine->flush();
        upload_vertex_data();
        return;
    }

    auto rect_primitive_t::set_image_view(image_view_t& image_view) -> void
    {
        auto gpu_guard{ m_engine->gpu_lock() };
        std::unique_lock lock{ m_mutex };
        m_engine->flush();
        m_image_view = &image_view;
        // Descriptor sets still reference the previous view until rebuilt.
        release_gpu();
        create_gpu();
        return;
    }

    auto rect_primitive_t::color_enable() -> void
    {
        auto gpu_guard{ m_engine->gpu_lock() };
        std::unique_lock lock{ m_mutex };
        if (!m_texture_enable) return;
        m_engine->flush();
        m_texture_enable = false;
        // Solid and textured vertices have different layouts and buffer sizes.
        release_gpu();
        create_gpu();
        return;
    }

    auto rect_primitive_t::texture_enable() -> void
    {
        auto gpu_guard{ m_engine->gpu_lock() };
        std::unique_lock lock{ m_mutex };
        if (m_texture_enable) return;
        m_engine->flush();
        m_texture_enable = true;
        release_gpu();
        create_gpu();
        return;
    }

    auto rect_primitive_t::inside(pos_2D pos) -> bool
    {
        std::unique_lock lock{ m_mutex };
        return ::lgo::inside(m_pos, m_size, pos);
    }

    auto rect_primitive_t::log(std::uint32_t tab_count) const -> void
    {
        std::unique_lock lock{ m_mutex };
        log_file(tab(tab_count) + "{\n");
        log_file(tab(tab_count + 1) + "type: rect_primitive_t\n");
        log_file(tab(tab_count + 1) + "m_name: " + m_name + "\n");
        log_file(tab(tab_count + 1) + "m_pos: " + to_string(m_pos) + "\n");
        log_file(tab(tab_count + 1) + "m_size: " + to_string(m_size) + "\n");
        log_file(tab(tab_count + 1) + "m_depth: " + std::to_string(m_depth) + "\n");
        log_file(tab(tab_count + 1) + "m_texture_enable: " + std::to_string(m_texture_enable) + "\n");
        log_file(tab(tab_count + 1) + "m_color: " + to_string(m_color) + "\n");
        log_file(tab(tab_count) + "}\n");
        return;
    }

    auto rect_primitive_t::render(vk::raii::CommandBuffer& command_buffer) -> void
    {
        std::unique_lock lock{ m_mutex };

        // GUI coordinates are logical pixels; Vulkan scissors use framebuffer pixels.
        // Clamp before conversion so a shrinking window cannot produce negative Vulkan offsets.
        const auto window_size{ m_engine->get_window_size() };
        const auto framebuffer_size{ m_engine->get_framebuffer_size() };
        const auto clip_pos{ m_clip_pos ? *m_clip_pos : pos_2D{} };
        const auto clip_size{ m_clip_size ? *m_clip_size : window_size };
        const auto left{ std::clamp<std::int64_t>(clip_pos.x, 0, window_size.x) };
        const auto top{ std::clamp<std::int64_t>(clip_pos.y, 0, window_size.y) };
        const auto right{ std::clamp(clip_pos.x + clip_size.x, left, window_size.x) };
        const auto bottom{ std::clamp(clip_pos.y + clip_size.y, top, window_size.y) };
        if (left == right || top == bottom)
        {
            return;
        }
        const auto x{ left * framebuffer_size.x / window_size.x };
        const auto y{ top * framebuffer_size.y / window_size.y };
        vk::Rect2D scissor_rect
        {
            .offset{ static_cast<std::int32_t>(x), static_cast<std::int32_t>(y) },
            .extent
            {
                static_cast<std::uint32_t>(right * framebuffer_size.x / window_size.x - x),
                static_cast<std::uint32_t>(bottom * framebuffer_size.y / window_size.y - y)
            }
        };
        if (m_texture_enable)
        {
            command_buffer.bindDescriptorSets
            (
                vk::PipelineBindPoint::eGraphics,
                m_texture_pipeline_state.layout_get(),
                0,
                *m_descriptor_set,
                nullptr
            );
        }

        command_buffer.setScissor(0, scissor_rect);

        command_buffer.bindVertexBuffers(0, *m_vertex_buffer.interface_get(), { 0 });
        command_buffer.draw(4, 1, 0, 0);
        return;
    }
}
