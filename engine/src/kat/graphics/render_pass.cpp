#include "kat/graphics/render_pass.hpp"

#include <ranges>

namespace kat {
    RenderPass::RenderPass(const std::shared_ptr<kat::Context>& context, const RenderPass::Description &desc) : m_context(context) {
        std::vector<vk::AttachmentDescription2> attachments;
        attachments.reserve(desc.attachments.size());
        for (const auto& attc : desc.attachments) {
            attachments.push_back(attc.to_vulkan_desc());
        }

        std::vector<vk::SubpassDescription2> subpasses;
        subpasses.reserve(desc.subpasses.size());
        for (const auto& subp : desc.subpasses) {
            subpasses.push_back(subp.to_vulkan_desc());
        };

        std::vector<vk::SubpassDependency2> dependencies;
        dependencies.reserve(desc.dependencies.size());
        for (const auto& dep : desc.dependencies) {
            dependencies.push_back(dep.to_vulkan_desc());
        }

        vk::RenderPassCreateInfo2 ci{};
        ci.setAttachments(attachments);
        ci.setSubpasses(subpasses);
        ci.setDependencies(dependencies);

        m_render_pass = m_context->device().createRenderPass2(ci);
    }

    RenderPass::~RenderPass() {
        m_context->device().destroy(m_render_pass);
    }

    void RenderPass::begin(const vk::CommandBuffer &cmd, const BeginInfo &begin_info) {
        std::vector<vk::ClearValue> cvs;
        cvs.reserve(begin_info.clear_values.size());
        for (const auto &cv : begin_info.clear_values) {
            switch (cv.index()) {
            case 0:
                cvs.push_back(std::get<kat::color>(cv).to_clear_value());
                break;
            case 1:
                cvs.push_back(std::get<vk::ClearDepthStencilValue>(cv));
                break;
            }
        }

        vk::RenderPassBeginInfo bi{};
        bi.setClearValues(cvs);
        bi.framebuffer = begin_info.framebuffer;
        bi.renderArea  = begin_info.render_area;
        bi.renderPass  = m_render_pass;

        cmd.beginRenderPass(bi, begin_info.contents);
    }

    void RenderPass::end(const vk::CommandBuffer &cmd) {
        cmd.endRenderPass();
    }

    std::vector<vk::Framebuffer> RenderPass::create_framebuffers(const std::vector<vk::ImageView> &image_views, const vk::Extent2D &extent) const {
        std::vector<vk::Framebuffer> framebuffers;
        framebuffers.reserve(image_views.size());

        for (const auto& image_view : image_views) {
            framebuffers.push_back(create_framebuffer(image_view, extent));
        }

        return framebuffers;
    }

    vk::Framebuffer RenderPass::create_framebuffer(const vk::ImageView &image_view, const vk::Extent2D &extent) const {
        return m_context->device().createFramebuffer(vk::FramebufferCreateInfo({}, m_render_pass, image_view, extent.width, extent.height, 1));
    }
} // namespace kat
