#pragma once

#include "kat/graphics/context.hpp"

#include <variant>
#include <vector>

#include <glm/glm.hpp>

namespace kat {

    struct AttachmentLayout {
        vk::Format              format;
        vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;

        vk::AttachmentLoadOp  load_op  = vk::AttachmentLoadOp::eClear;
        vk::AttachmentStoreOp store_op = vk::AttachmentStoreOp::eStore;

        vk::AttachmentLoadOp  stencil_load_op  = vk::AttachmentLoadOp::eDontCare;
        vk::AttachmentStoreOp stencil_store_op = vk::AttachmentStoreOp::eDontCare;

        vk::ImageLayout initial_layout = vk::ImageLayout::eUndefined;
        vk::ImageLayout final_layout   = vk::ImageLayout::eGeneral;

        constexpr vk::AttachmentDescription2 to_vulkan_desc() const noexcept {
            return vk::AttachmentDescription2{{}, format, samples, load_op, store_op, stencil_load_op, stencil_store_op, initial_layout, final_layout};
        };
    };

    struct Subpass {
        std::vector<vk::AttachmentReference2>   color_attachments;
        std::vector<vk::AttachmentReference2>   input_attachments;
        std::vector<vk::AttachmentReference2>   resolve_attachments;
        std::optional<vk::AttachmentReference2> depth_stencil_attachment;
        std::vector<uint32_t>                   preserve_attachments;

        vk::PipelineBindPoint bind_point = vk::PipelineBindPoint::eGraphics;
        uint32_t              view_mask  = 0;

        inline vk::SubpassDescription2 to_vulkan_desc() const noexcept {
            return vk::SubpassDescription2({}, bind_point, view_mask, input_attachments, color_attachments, resolve_attachments, ptr_to_optional(depth_stencil_attachment),
                                           preserve_attachments);
        };
    };

    struct SubpassDependency {
        static constexpr uint32_t SUBPASS_EXTERNAL = VK_SUBPASS_EXTERNAL;

        uint32_t src_subpass;
        uint32_t dst_subpass;

        vk::PipelineStageFlags src_stage_mask;
        vk::PipelineStageFlags dst_stage_mask;

        vk::AccessFlags src_access_mask = vk::AccessFlagBits::eNone;
        vk::AccessFlags dst_access_mask = vk::AccessFlagBits::eNone;

        vk::DependencyFlags dependency_flags;

        int32_t view_offset = 0;

        constexpr vk::SubpassDependency2 to_vulkan_desc() const noexcept {
            return vk::SubpassDependency2{src_subpass, dst_subpass, src_stage_mask, dst_stage_mask, src_access_mask, dst_access_mask, dependency_flags, view_offset};
        };
    };

    class RenderPass {
      public:
        struct Description {
            std::vector<AttachmentLayout>  attachments;
            std::vector<Subpass>           subpasses;
            std::vector<SubpassDependency> dependencies;
        };

        using ClearValue = std::variant<kat::color, vk::ClearDepthStencilValue>;

        struct BeginInfo {
            std::vector<ClearValue> clear_values;
            vk::Rect2D              render_area;
            vk::Framebuffer         framebuffer;
            vk::SubpassContents     contents = vk::SubpassContents::eInline;
        };

        RenderPass(const std::shared_ptr<kat::Context> &context, const Description &desc);

        ~RenderPass();

        void begin(const vk::CommandBuffer &cmd, const BeginInfo &begin_info);
        void end(const vk::CommandBuffer &cmd);

        [[nodiscard]] std::vector<vk::Framebuffer> create_framebuffers(const std::vector<vk::ImageView> &image_views, const vk::Extent2D &extent) const;

        [[nodiscard]] vk::Framebuffer create_framebuffer(const vk::ImageView &image_view, const vk::Extent2D &extent) const;

        [[nodiscard]] inline vk::RenderPass handle() const { return m_render_pass; };

      private:
        std::shared_ptr<kat::Context> m_context;
        vk::RenderPass                m_render_pass;
    };
} // namespace kat
