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
    descriptor_set_layout_t rect_primitive_t::m_solid_descriptor_set_layout{};
    descriptor_set_layout_t rect_primitive_t::m_texture_descriptor_set_layout{};
    pipeline_state_t rect_primitive_t::m_solid_pipeline_state{};
    pipeline_state_t rect_primitive_t::m_texture_pipeline_state{};
    vk::raii::Sampler rect_primitive_t::m_sampler{ nullptr };

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
            .device_physical{ &m_engine->device_physical_get() },
            .device{ &m_engine->device_get() },
            .usage{ vk::BufferUsageFlagBits::eVertexBuffer },
            .property{ vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent },
            .size{ m_vertex_buffer_size }
        });
        return;
    }

    auto rect_primitive_t::upload_vertex_data() -> void
    {
        std::unique_lock lock{ m_mutex };
        float pos_x{ static_cast<float>(m_pos.x) / static_cast<float>(m_engine->get_window_size().x) * 2.0f - 1.0f };
        float pos_y{ static_cast<float>(m_pos.y) / static_cast<float>(m_engine->get_window_size().y) * 2.0f - 1.0f };
        float size_x{ static_cast<float>(m_size.x) / static_cast<float>(m_engine->get_window_size().x) * 2.0f };
        float size_y{ static_cast<float>(m_size.y) / static_cast<float>(m_engine->get_window_size().y) * 2.0f };

        //texture_index is not used.
        std::uint32_t texture_index{ 0 };
        if (m_texture_enable)
        {
            auto vertex_data
            {
                std::to_array<texture_vertex_t>
                ({
                    { { pos_x, pos_y, m_depth }, texture_index, { 0.0f, 0.0f } },
                    { { pos_x + size_x, pos_y, m_depth }, texture_index, { 1.0f, 0.0f } },
                    { { pos_x, pos_y + size_y, m_depth }, texture_index, { 0.0f, 1.0f } },
                    { { pos_x + size_x, pos_y + size_y, m_depth }, texture_index, { 1.0f, 1.0f } }
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

        m_solid_descriptor_set_layout.serialize(m_engine->device_get());
        m_solid_pipeline_state.init
        ({
            .name{ "rect_primitive_t::solid_pipeline_state" },
            .descriptor_set_layout{ &m_solid_descriptor_set_layout },
            .topology{ vk::PrimitiveTopology::eTriangleStrip },
            .surface_format{ m_engine->format_get() }
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
        shader_module_t solid_shader_module{ engine->device_get(), "solid.spv" };
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
            .surface_format{ m_engine->format_get() }
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
        shader_module_t texture_shader_module{ engine->device_get(), "texture.spv" };
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
        vk::SamplerCreateInfo sampler_info
        {
            .magFilter{ vk::Filter::eLinear },
            .minFilter{ vk::Filter::eLinear },
            .mipmapMode{ vk::SamplerMipmapMode::eLinear },
            .addressModeU{ vk::SamplerAddressMode::eRepeat },
            .addressModeV{ vk::SamplerAddressMode::eRepeat },
            .addressModeW{ vk::SamplerAddressMode::eRepeat },
            .mipLodBias{ 0.0f },
            .anisotropyEnable{ vk::False },
            .maxAnisotropy{ device_physical_property.limits.maxSamplerAnisotropy },
            .compareEnable{ vk::False },
            .compareOp{ vk::CompareOp::eAlways },
            .unnormalizedCoordinates{ vk::False }
        };
        m_sampler = vk::raii::Sampler{ engine->device_get(), sampler_info };

        return;
    }

    auto rect_primitive_t::uninit() -> void {}

    auto rect_primitive_t::render
    (
        const std::set<rect_primitive_t*>& rect,
        vk::raii::CommandBuffer& command_buffer
    )
    -> void
    {
        command_buffer.bindPipeline
        (
            vk::PipelineBindPoint::eGraphics,
            *m_solid_pipeline_state.interface_get()
        );
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
        return;
    }

    rect_primitive_t::rect_primitive_t(const ctor_t& ctor)
    :
        m_pos{ ctor.pos },
        m_size{ ctor.size },
        m_depth{ ctor.depth },
        m_clip_pos{ ctor.clip_pos ? ctor.clip_pos.value() : ctor.pos },
        m_clip_size{ ctor.clip_size ? ctor.clip_size.value() : ctor.size },
        m_color{ ctor.color },
        m_image_view{ ctor.image_view },
        m_texture_pos{ ctor.texture_pos },
        m_texture_axis_x{ ctor.texture_axis_x },
        m_texture_axis_y{ ctor.texture_axis_y },
        m_name{ ctor.name }
    {
        m_texture_enable = ctor.image_view;

        create_vertex_buffer();
        upload_vertex_data();

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

            std::vector<vk::DescriptorSetLayout> descriptor_set_layout( 1, *m_texture_descriptor_set_layout.interface_get() );
            vk::DescriptorSetAllocateInfo descriptor_set_info
            {
                .descriptorPool{ m_descriptor_pool },
                .descriptorSetCount{ static_cast<std::uint32_t>(descriptor_set_layout.size()) },
                .pSetLayouts{ descriptor_set_layout.data() }
            };
            m_descriptor_set = std::move(m_engine->device_get().allocateDescriptorSets(descriptor_set_info)[0]);

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

    auto rect_primitive_t::set_pos(pos_2D pos) -> void
    {
        std::unique_lock lock{ m_mutex };
        m_clip_pos = m_clip_pos + (pos - m_pos);
        m_pos = pos;
        m_engine->flush();
        upload_vertex_data();
        m_engine->draw();
        return;
    }

    auto rect_primitive_t::set_size(size_2D size) -> void
    {
        std::unique_lock lock{ m_mutex };
        m_size = size;
        m_engine->flush();
        upload_vertex_data();
        return;
    }

    auto rect_primitive_t::set_color(color_t color) -> void
    {
        std::unique_lock lock{ m_mutex };
        m_color = color;
        m_engine->flush();
        upload_vertex_data();
        return;
    }

    auto rect_primitive_t::set_image_view(image_view_t& image_view) -> void
    {
        std::unique_lock lock{ m_mutex };
        m_image_view = &image_view;
        m_engine->flush();
        upload_vertex_data();
        return;
    }

    auto rect_primitive_t::color_enable() -> void
    {
        std::unique_lock lock{ m_mutex };
        m_texture_enable = false;
        return;
    }

    auto rect_primitive_t::texture_enable() -> void
    {
        std::unique_lock lock{ m_mutex };
        m_texture_enable = true;
        return;
    }

    auto rect_primitive_t::inside(pos_2D pos) -> bool
    {
        std::unique_lock lock{ m_mutex };
        return ::lgo::inside(m_pos, m_size, pos);
    }

    auto rect_primitive_t::log(std::uint32_t tab_count) const -> void
    {
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

        //(0, 0) is at the top left corner of the window.
        vk::Rect2D scissor_rect
        {
            .offset
            {
                .x{ static_cast<std::int32_t>(m_clip_pos.x) },
                .y{ static_cast<std::int32_t>(m_clip_pos.y) }
            },
            .extent
            {
                .width{ static_cast<std::uint32_t>(m_clip_size.x) },
                .height{ static_cast<std::uint32_t>(m_clip_size.y) }
            }
        };
        command_buffer.setScissor(0, scissor_rect);

        command_buffer.bindVertexBuffers(0, *m_vertex_buffer.interface_get(), { 0 });
        command_buffer.draw(4, 1, 0, 0);
        return;
    }
}
