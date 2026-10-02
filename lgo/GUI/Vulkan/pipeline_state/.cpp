module lgo.GUI.Vulkan;

import std;

import external.Vulkan;

namespace lgo
{
    pipeline_state_t::pipeline_state_t(const ctor_t& ctor)
    {
        init(ctor);
        return;
    }

    auto pipeline_state_t::init(const ctor_t& ctor) -> void
    {
        m_name = ctor.name;
        m_descriptor_set_layout = ctor.descriptor_set_layout;
        m_input_assembly.topology = ctor.topology;
        m_surface_format = ctor.surface_format;
        return;
    }

    auto pipeline_state_t::shader_add(shader_t& shader) -> void
    {
        m_shader.push_back
        (
            vk::PipelineShaderStageCreateInfo
            {
                .stage{ shader.stage_get() },
                .module{ shader.module_get().interface_get() },
                .pName{ shader.name_get().c_str() }
            }
        );
        return;
    }

    auto pipeline_state_t::vertex_binding_add
    (
        std::uint32_t binding,
        std::size_t stride,
        vk::VertexInputRate input_rate
    )
    -> void
    {
        m_vertex_binding.push_back
        (
            vk::VertexInputBindingDescription
            {
                .binding{ binding },
                .stride{ static_cast<std::uint32_t>(stride) },
                .inputRate{ input_rate }
            }
        );
        return;
    }

    auto pipeline_state_t::vertex_attribute_add
    (
        std::uint32_t location,
        std::uint32_t binding,
        vk::Format format,
        std::uint32_t offset
    )
    -> void
    {
        m_vertex_attribute.push_back
        (
            vk::VertexInputAttributeDescription
            {
                .location{ location },
                .binding{ binding },
                .format{ format },
                .offset{ offset }
            }
        );
        return;
    }

    auto pipeline_state_t::serialize(vk::raii::Device& device) -> void
    {
        std::vector<vk::DynamicState> dynamic_state_arr
        {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor
        };
        vk::PipelineDynamicStateCreateInfo dynamic_state_info
        {
            .dynamicStateCount{ static_cast<std::uint32_t>(dynamic_state_arr.size()) },
            .pDynamicStates{ dynamic_state_arr.data() }
        };
        vk::PipelineVertexInputStateCreateInfo vertex_input_info
        {
            .vertexBindingDescriptionCount{ static_cast<std::uint32_t>(m_vertex_binding.size()) },
            .pVertexBindingDescriptions{ m_vertex_binding.data() },
            .vertexAttributeDescriptionCount{ static_cast<std::uint32_t>(m_vertex_attribute.size()) },
            .pVertexAttributeDescriptions{ m_vertex_attribute.data() }
        };
        vk::PipelineViewportStateCreateInfo viewport_state
        {
            .viewportCount{ 1 },
            .scissorCount{ 1 }
        };
        //Do not draw triangles that are back-facing.
        //Vulkan coordinate system:
        //+x: to the right
        //+y: downward
        //+z: out of the screen
        vk::PipelineRasterizationStateCreateInfo rasterization
        {
            .depthClampEnable{ vk::False },
            .rasterizerDiscardEnable{ vk::False },
            .polygonMode{ vk::PolygonMode::eFill },
            .cullMode{ vk::CullModeFlagBits::eBack },
            .frontFace{ vk::FrontFace::eCounterClockwise },
            .depthBiasEnable{ vk::False },
            .lineWidth{ 1.0f }
        };
        vk::PipelineMultisampleStateCreateInfo multisample
        {
            .rasterizationSamples{ vk::SampleCountFlagBits::e1 },
            .sampleShadingEnable{ vk::False }
        };
        vk::PipelineColorBlendAttachmentState color_blend_arr
        {
            .blendEnable{ vk::False },
            .colorWriteMask
            {
                vk::ColorComponentFlagBits::eR |
                vk::ColorComponentFlagBits::eG |
                vk::ColorComponentFlagBits::eB |
                vk::ColorComponentFlagBits::eA
            }
        };
        vk::PipelineColorBlendStateCreateInfo color_blend
        {
            .logicOpEnable{ vk::False },
            .attachmentCount{ 1 },
            .pAttachments{ &color_blend_arr }
        };
        vk::PipelineLayoutCreateInfo pipeline_layout_info
        {
            .setLayoutCount{ 1 },
            .pSetLayouts{ &*m_descriptor_set_layout->interface_get() },
            .pushConstantRangeCount{ 0 }
        };
        m_pipeline_layout = vk::raii::PipelineLayout{ device, pipeline_layout_info };
        vk::StructureChain
        <
            vk::GraphicsPipelineCreateInfo,
            vk::PipelineRenderingCreateInfo
        >
        pipeline_info
        {
            {
                .stageCount{ static_cast<std::uint32_t>(m_shader.size()) },
                .pStages{ m_shader.data() },
                .pVertexInputState{ &vertex_input_info },
                .pInputAssemblyState{ &m_input_assembly },
                .pViewportState{ &viewport_state },
                .pRasterizationState{ &rasterization },
                .pMultisampleState{ &multisample },
                .pColorBlendState{ &color_blend },
                .pDynamicState{ &dynamic_state_info },
                .layout{ m_pipeline_layout },
                .renderPass{ nullptr }
            },
            {
                .colorAttachmentCount{ 1 },
                .pColorAttachmentFormats{ &m_surface_format }
            }
        };
        m_interface = vk::raii::Pipeline
        {
            device,
            nullptr,
            pipeline_info.get<vk::GraphicsPipelineCreateInfo>()
        };
        return;
    }

    auto pipeline_state_t::interface_get() -> vk::raii::Pipeline&
    {
        return m_interface;
    }

    auto pipeline_state_t::layout_get() -> vk::raii::PipelineLayout&
    {
        return m_pipeline_layout;
    }
}
