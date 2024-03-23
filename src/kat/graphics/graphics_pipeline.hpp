#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

#include "kat/graphics/context.hpp"
#include "kat/graphics/render_pass.hpp"
#include "kat/graphics/shader_cache.hpp"

namespace kat {
    struct ShaderStage {
        kat::ShaderId           shader_id;
        vk::ShaderStageFlagBits stage;
        std::string             entry_point = "main";
    };

    struct VertexAttribute {
        uint32_t   location;
        vk::Format format;
        uint32_t   offset;
    };

    struct VertexBinding {
        uint32_t binding;
        uint32_t stride;

        std::vector<VertexAttribute> attributes;

        vk::VertexInputRate input_rate = vk::VertexInputRate::eVertex;
    };

    struct VertexLayout {
        std::vector<VertexBinding> bindings;
    };

    class PipelineLayout {
      public:
        struct Description {
            // todo descriptors

            std::vector<vk::PushConstantRange> push_constant_ranges;
        };

        PipelineLayout(const std::shared_ptr<Context> &context, const Description &desc);

        [[nodiscard]] inline vk::PipelineLayout handle() const { return m_pipeline_layout; };

      private:
        std::shared_ptr<Context> m_context;

        vk::PipelineLayout m_pipeline_layout;
    };

    class GraphicsPipeline {
      public:
        struct PrimitiveState {
            vk::PrimitiveTopology topology                 = vk::PrimitiveTopology::eTriangleList;
            bool                  enable_primitive_restart = false;
        };

        struct RasterizerState {
            bool              enable_depth_clamp         = false;
            bool              discard_rasterizer_output  = false;
            vk::PolygonMode   polygon_mode               = vk::PolygonMode::eFill;
            float             line_width                 = 1.0f;
            vk::CullModeFlags cull_mode                  = vk::CullModeFlagBits::eBack;
            vk::FrontFace     front_face                 = vk::FrontFace::eClockwise;
            bool              enable_depth_bias          = false;
            float             depth_bias_constant_factor = 0.0f;
            float             depth_bias_clamp           = 0.0f;
            float             depth_bias_slope_factor    = 0.0f;
        };

        struct MultisampleState {
            bool                    enable_sample_shading = false;
            vk::SampleCountFlagBits rasterization_samples = vk::SampleCountFlagBits::e1;
            float                   min_sample_shading    = 1.0f;
            std::vector<uint32_t>   sample_mask;
            bool                    enable_alpha_to_coverage = false;
            bool                    enable_alpha_to_one      = false;
        };

        struct DepthStencilState {
            bool               enable_depth_test        = false;
            bool               enable_depth_write       = false;
            vk::CompareOp      depth_compare_op         = vk::CompareOp::eLess;
            bool               enable_depth_bounds_test = false;
            bool               enable_stencil_test      = false;
            vk::StencilOpState stencil_front{};
            vk::StencilOpState stencil_back{};
            float              min_depth_bound = 0.0f;
            float              max_depth_bound = 0.0f;
        };

        struct BlendState {
            bool                                               enable_logic_op = false;
            vk::LogicOp                                        logic_op        = vk::LogicOp::eCopy;
            std::vector<vk::PipelineColorBlendAttachmentState> blend_attachments;
            std::array<float, 4>                               blend_constants = {0.0f, 0.0f, 0.0f, 0.0f};
        };

        struct Description {
            std::vector<ShaderStage>        shader_stages;
            VertexLayout                    vertex_layout{};
            PrimitiveState                  primitive_state{};
            RasterizerState                 rasterizer_state{};
            MultisampleState                multisample_state{};
            DepthStencilState               depth_stencil_state{};
            BlendState                      blend_state{};
            uint32_t                        patch_control_points = 1;
            std::vector<vk::Viewport>       viewports;
            std::vector<vk::Rect2D>         scissors;
            std::vector<vk::DynamicState>   dynamic_states;
            std::shared_ptr<PipelineLayout> layout;
            std::shared_ptr<RenderPass>     render_pass;
            uint32_t                        subpass;

            Description& add_shader(const ShaderId& id, vk::ShaderStageFlagBits stage, const std::string& entry_point = "main");
        };

        GraphicsPipeline(const std::shared_ptr<Context> &context, const Description &desc);

        [[nodiscard]] inline vk::Pipeline handle() const { return m_pipeline; };

        void bind(const vk::CommandBuffer& cmd) const;

      private:
        std::shared_ptr<Context> m_context;

        vk::Pipeline m_pipeline;
    };
} // namespace kat
