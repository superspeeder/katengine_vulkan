#include "kat/graphics/graphics_pipeline.hpp"
#include "graphics_pipeline.hpp"

#include <iostream>

namespace kat {

    PipelineLayout::PipelineLayout(const std::shared_ptr<Context> &context, const Description &desc) : m_context(context) {
        vk::PipelineLayoutCreateInfo ci{};

        m_pipeline_layout = m_context->device().createPipelineLayout(ci);
    }

    GraphicsPipeline::GraphicsPipeline(const std::shared_ptr<Context> &context, const Description &desc) : m_context(context) {
        vk::GraphicsPipelineCreateInfo ci{};

        std::vector<vk::PipelineShaderStageCreateInfo> shader_stages;
        shader_stages.reserve(desc.shader_stages.size());
        for (const auto &stage : desc.shader_stages) {
            shader_stages.push_back(vk::PipelineShaderStageCreateInfo({}, stage.stage, m_context->shader_cache()->get(stage.shader_id), stage.entry_point.c_str()));
        }

        std::cout << shader_stages.size() << " stages\n";

        vk::PipelineVertexInputStateCreateInfo vertex_input{};

        std::vector<vk::VertexInputAttributeDescription> attribs;
        std::vector<vk::VertexInputBindingDescription>   bindings;

        for (const auto &binding : desc.vertex_layout.bindings) {
            bindings.push_back(vk::VertexInputBindingDescription(binding.binding, binding.stride, binding.input_rate));
            for (const auto &attrib : binding.attributes) {
                attribs.push_back(vk::VertexInputAttributeDescription(attrib.location, binding.binding, attrib.format, attrib.offset));
            }
        }

        vertex_input.setVertexAttributeDescriptions(attribs);
        vertex_input.setVertexBindingDescriptions(bindings);

        vk::PipelineInputAssemblyStateCreateInfo input_assembly{};
        input_assembly.primitiveRestartEnable = desc.primitive_state.enable_primitive_restart;
        input_assembly.topology               = desc.primitive_state.topology;

        vk::PipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.depthClampEnable        = desc.rasterizer_state.enable_depth_clamp;
        rasterizer.rasterizerDiscardEnable = desc.rasterizer_state.discard_rasterizer_output;
        rasterizer.polygonMode             = desc.rasterizer_state.polygon_mode;
        rasterizer.lineWidth               = desc.rasterizer_state.line_width;
        rasterizer.cullMode                = desc.rasterizer_state.cull_mode;
        rasterizer.frontFace               = desc.rasterizer_state.front_face;
        rasterizer.depthBiasEnable         = desc.rasterizer_state.enable_depth_bias;
        rasterizer.depthBiasConstantFactor = desc.rasterizer_state.depth_bias_constant_factor;
        rasterizer.depthBiasSlopeFactor    = desc.rasterizer_state.depth_bias_slope_factor;
        rasterizer.depthBiasClamp          = desc.rasterizer_state.depth_bias_clamp;

        vk::PipelineMultisampleStateCreateInfo multisample{};
        multisample.sampleShadingEnable   = desc.multisample_state.enable_sample_shading;
        multisample.rasterizationSamples  = desc.multisample_state.rasterization_samples;
        multisample.minSampleShading      = desc.multisample_state.min_sample_shading;
        multisample.pSampleMask           = desc.multisample_state.sample_mask.data();
        multisample.alphaToCoverageEnable = desc.multisample_state.enable_alpha_to_coverage;
        multisample.alphaToOneEnable      = desc.multisample_state.enable_alpha_to_one;

        vk::PipelineDepthStencilStateCreateInfo depth_stencil{};
        depth_stencil.depthTestEnable       = desc.depth_stencil_state.enable_depth_test;
        depth_stencil.depthWriteEnable      = desc.depth_stencil_state.enable_depth_write;
        depth_stencil.depthCompareOp        = desc.depth_stencil_state.depth_compare_op;
        depth_stencil.depthBoundsTestEnable = desc.depth_stencil_state.enable_depth_bounds_test;
        depth_stencil.stencilTestEnable     = desc.depth_stencil_state.enable_stencil_test;
        depth_stencil.front                 = desc.depth_stencil_state.stencil_front;
        depth_stencil.back                  = desc.depth_stencil_state.stencil_back;
        depth_stencil.minDepthBounds        = desc.depth_stencil_state.min_depth_bound;
        depth_stencil.maxDepthBounds        = desc.depth_stencil_state.max_depth_bound;

        vk::PipelineColorBlendStateCreateInfo blend{};
        blend.logicOp       = desc.blend_state.logic_op;
        blend.logicOpEnable = desc.blend_state.enable_logic_op;
        blend.setBlendConstants(desc.blend_state.blend_constants);
        blend.setAttachments(desc.blend_state.blend_attachments);

        vk::PipelineTessellationStateCreateInfo tessellator{};
        tessellator.patchControlPoints = desc.patch_control_points;

        vk::PipelineViewportStateCreateInfo viewport{};
        viewport.setViewports(desc.viewports);
        viewport.setScissors(desc.scissors);

        vk::PipelineDynamicStateCreateInfo dynamic_state{};
        dynamic_state.setDynamicStates(desc.dynamic_states);

        ci.setStages(shader_stages);
        ci.pVertexInputState   = &vertex_input;
        ci.pInputAssemblyState = &input_assembly;
        ci.pRasterizationState = &rasterizer;
        ci.pMultisampleState   = &multisample;
        ci.pDepthStencilState  = &depth_stencil;
        ci.pColorBlendState    = &blend;
        ci.pTessellationState  = &tessellator;
        ci.pViewportState      = &viewport;
        ci.pDynamicState       = &dynamic_state;
        ci.layout              = desc.layout->handle();
        ci.renderPass          = desc.render_pass->handle();
        ci.subpass             = desc.subpass;

        m_pipeline = m_context->device().createGraphicsPipeline(m_context->pipeline_cache(), ci).value;
    }

    void GraphicsPipeline::bind(const vk::CommandBuffer &cmd) const {
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);
    }

    GraphicsPipeline::Description &GraphicsPipeline::Description::add_shader(const ShaderId &id, vk::ShaderStageFlagBits stage, const std::string &entry_point) {
        shader_stages.emplace_back(id, stage, entry_point);
        return *this;
    }
} // namespace kat
