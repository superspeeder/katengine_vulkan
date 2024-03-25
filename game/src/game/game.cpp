#include "game.hpp"

#include <glm/gtx/string_cast.hpp>
#include <iostream>



namespace game {
    Game::Game() : kat::App({.title = "Window", .fullscreen = true}, {}) {
        m_command_pool    = m_context->create_command_pool_raw<kat::QueueType::GRAPHICS>();
        m_command_buffers = m_context->allocate_command_buffers_raw<kat::MAX_FRAMES_IN_FLIGHT>(m_command_pool);

        create_render_pass();
        create_buffers();
        create_pipeline_layout();
        create_graphics_pipeline();
    }

    void Game::create_render_pass() {
        kat::RenderPass::Description desc{};

        desc.attachments.push_back(kat::AttachmentLayout{
            .format       = m_context->swapchain_format(),
            .final_layout = vk::ImageLayout::ePresentSrcKHR,
        });

        desc.subpasses.push_back(kat::Subpass{
            .color_attachments =
                {
                    vk::AttachmentReference2(0, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageAspectFlagBits::eColor),
                },
        });

        desc.dependencies.push_back(kat::SubpassDependency{
            .src_subpass = kat::SubpassDependency::SUBPASS_EXTERNAL,
            .dst_subpass = 0,

            .src_stage_mask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
            .dst_stage_mask = vk::PipelineStageFlagBits::eColorAttachmentOutput,

            .src_access_mask = vk::AccessFlagBits::eNone,
            .dst_access_mask = vk::AccessFlagBits::eColorAttachmentWrite,
        });

        m_render_pass  = std::make_shared<kat::RenderPass>(m_context, desc);
        m_framebuffers = m_render_pass->create_framebuffers(m_context->swapchain_image_views(), m_context->swapchain_extent());
    }

    void Game::create_pipeline_layout() {
        {
            kat::DescriptorSetLayout::Description desc{};
            desc.bindings = {vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eAllGraphics, {})};

            m_descriptor_set_layout = std::make_shared<kat::DescriptorSetLayout>(m_context, desc);
        }

        {
            kat::PipelineLayout::Description desc{};

            desc.push_constant_ranges   = {vk::PushConstantRange(vk::ShaderStageFlagBits::eAllGraphics, 0, sizeof(PushConstants))};
            desc.descriptor_set_layouts = {m_descriptor_set_layout};

            m_pipeline_layout = std::make_shared<kat::PipelineLayout>(m_context, desc);
        }

        {
            kat::DescriptorPool::Description desc{};
            desc.max_sets     = kat::MAX_FRAMES_IN_FLIGHT;
            desc.pool_sizes   = {vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, kat::MAX_FRAMES_IN_FLIGHT)};
            m_descriptor_pool = std::make_shared<kat::DescriptorPool>(m_context, desc);
        }

        m_descriptor_sets = m_descriptor_pool->allocate_sets(m_descriptor_set_layout, kat::MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < kat::MAX_FRAMES_IN_FLIGHT; i++) {
            vk::DescriptorBufferInfo dbi{};
            dbi.buffer = m_uniform_buffers[i]->handle();
            dbi.offset = 0;
            dbi.range  = sizeof(UniformBuffer);

            vk::WriteDescriptorSet write{};
            write.dstSet          = m_descriptor_sets[i];
            write.dstBinding      = 0;
            write.dstArrayElement = 0;
            write.descriptorType  = vk::DescriptorType::eUniformBuffer;
            write.setBufferInfo(dbi);

            m_context->device().updateDescriptorSets(write, {});
        }
    }

    void Game::create_graphics_pipeline() {
        kat::GraphicsPipeline::Description desc{};

        desc.add_shader("shaders/main.vert.spv", vk::ShaderStageFlagBits::eVertex);
        desc.add_shader("shaders/main.frag.spv", vk::ShaderStageFlagBits::eFragment);

        desc.blend_state.blend_attachments = {
            kat::STANDARD_BLEND_STATE,
        };

        desc.viewports.push_back(m_context->full_viewport());
        desc.scissors.push_back(m_context->full_render_area());

        desc.vertex_layout.bindings = {kat::VertexBinding{0,
                                                          sizeof(Vertex),
                                                          {
                                                              kat::VertexAttribute{0, vk::Format::eR32G32B32Sfloat, 0},
                                                              kat::VertexAttribute{1, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, color)},
                                                              kat::VertexAttribute{2, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)},
                                                          },
                                                          vk::VertexInputRate::eVertex}};

        desc.layout      = m_pipeline_layout;
        desc.render_pass = m_render_pass;
        desc.subpass     = 0;

        m_graphics_pipeline = std::make_shared<kat::GraphicsPipeline>(m_context, desc);
    }

    void Game::create_buffers() {
        const std::vector<Vertex> vertices = {
            // back
            Vertex{glm::vec3(-0.5f, 0.5f, -0.5f), kat::BLACK, glm::vec3(0.0f, -0.0f, -1.0f)},
            Vertex{glm::vec3(0.5f, 0.5f, -0.5f), kat::RED, glm::vec3(0.0f, -0.0f, -1.0f)},
            Vertex{glm::vec3(0.5f, -0.5f, -0.5f), kat::GREEN, glm::vec3(0.0f, -0.0f, -1.0f)},
            Vertex{glm::vec3(0.5f, -0.5f, -0.5f), kat::GREEN, glm::vec3(0.0f, -0.0f, -1.0f)},
            Vertex{glm::vec3(-0.5f, -0.5f, -0.5f), kat::BLUE, glm::vec3(0.0f, -0.0f, -1.0f)},
            Vertex{glm::vec3(-0.5f, 0.5f, -0.5f), kat::BLACK, glm::vec3(0.0f, -0.0f, -1.0f)},

            // front
            Vertex{glm::vec3(0.5f, 0.5f, 0.5f), kat::CYAN, glm::vec3(0.0f, -0.0f, 1.0f)},
            Vertex{glm::vec3(-0.5f, 0.5f, 0.5f), kat::WHITE, glm::vec3(0.0f, -0.0f, 1.0f)},
            Vertex{glm::vec3(0.5f, -0.5f, 0.5f), kat::MAGENTA, glm::vec3(0.0f, -0.0f, 1.0f)},
            Vertex{glm::vec3(0.5f, -0.5f, 0.5f), kat::MAGENTA, glm::vec3(0.0f, -0.0f, 1.0f)},
            Vertex{glm::vec3(-0.5f, 0.5f, 0.5f), kat::WHITE, glm::vec3(0.0f, -0.0f, 1.0f)},
            Vertex{glm::vec3(-0.5f, -0.5f, 0.5f), kat::YELLOW, glm::vec3(0.0f, -0.0f, 1.0f)},

            // left
            Vertex{glm::vec3(-0.5f, -0.5f, -0.5f), kat::BLUE, glm::vec3(-1.0f, -0.0f, 0.0f)},
            Vertex{glm::vec3(-0.5f, -0.5f, 0.5f), kat::YELLOW, glm::vec3(-1.0f, -0.0f, 0.0f)},
            Vertex{glm::vec3(-0.5f, 0.5f, -0.5f), kat::BLACK, glm::vec3(-1.0f, -0.0f, 0.0f)},
            Vertex{glm::vec3(-0.5f, 0.5f, -0.5f), kat::BLACK, glm::vec3(-1.0f, -0.0f, 0.0f)},
            Vertex{glm::vec3(-0.5f, -0.5f, 0.5f), kat::YELLOW, glm::vec3(-1.0f, -0.0f, 0.0f)},
            Vertex{glm::vec3(-0.5f, 0.5f, 0.5f), kat::WHITE, glm::vec3(-1.0f, -0.0f, 0.0f)},

            // right
            Vertex{glm::vec3(0.5f, -0.5f, 0.5f), kat::MAGENTA, glm::vec3(1.0f, -0.0f, 0.0f)},
            Vertex{glm::vec3(0.5f, -0.5f, -0.5f), kat::GREEN, glm::vec3(1.0f, -0.0f, 0.0f)},
            Vertex{glm::vec3(0.5f, 0.5f, -0.5f), kat::RED, glm::vec3(1.0f, -0.0f, 0.0f)},
            Vertex{glm::vec3(0.5f, 0.5f, -0.5f), kat::RED, glm::vec3(1.0f, -0.0f, 0.0f)},
            Vertex{glm::vec3(0.5f, 0.5f, 0.5f), kat::CYAN, glm::vec3(1.0f, -0.0f, 0.0f)},
            Vertex{glm::vec3(0.5f, -0.5f, 0.5f), kat::MAGENTA, glm::vec3(1.0f, -0.0f, 0.0f)},

            // bottom
            Vertex{glm::vec3(0.5f, 0.5f, -0.5f), kat::RED, glm::vec3(0.0f, 1.0f, 0.0f)},
            Vertex{glm::vec3(-0.5f, 0.5f, -0.5f), kat::BLACK, glm::vec3(0.0f, 1.0f, 0.0f)},
            Vertex{glm::vec3(0.5f, 0.5f, 0.5f), kat::CYAN, glm::vec3(0.0f, 1.0f, 0.0f)},
            Vertex{glm::vec3(-0.5f, 0.5f, 0.5f), kat::WHITE, glm::vec3(0.0f, 1.0f, 0.0f)},
            Vertex{glm::vec3(0.5f, 0.5f, 0.5f), kat::CYAN, glm::vec3(0.0f, 1.0f, 0.0f)},
            Vertex{glm::vec3(-0.5f, 0.5f, -0.5f), kat::BLACK, glm::vec3(0.0f, 1.0f, 0.0f)},

            // top
            Vertex{glm::vec3(-0.5f, -0.5f, -0.5f), kat::BLUE, glm::vec3(0.0f, -1.0f, 0.0f)},
            Vertex{glm::vec3(0.5f, -0.5f, -0.5f), kat::GREEN, glm::vec3(0.0f, -1.0f, 0.0f)},
            Vertex{glm::vec3(0.5f, -0.5f, 0.5f), kat::MAGENTA, glm::vec3(0.0f, -1.0f, 0.0f)},
            Vertex{glm::vec3(0.5f, -0.5f, 0.5f), kat::MAGENTA, glm::vec3(0.0f, -1.0f, 0.0f)},
            Vertex{glm::vec3(-0.5f, -0.5f, 0.5f), kat::YELLOW, glm::vec3(0.0f, -1.0f, 0.0f)},
            Vertex{glm::vec3(-0.5f, -0.5f, -0.5f), kat::BLUE, glm::vec3(0.0f, -1.0f, 0.0f)},
        };

        m_vertex_buffer = m_context->gpu_allocator()->init_buffer(vertices, vk::BufferUsageFlagBits::eVertexBuffer, VMA_MEMORY_USAGE_GPU_ONLY);

        const std::vector<uint32_t> indices = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17,
                                               18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35};

        m_index_buffer = m_context->gpu_allocator()->init_buffer(indices, vk::BufferUsageFlagBits::eIndexBuffer, VMA_MEMORY_USAGE_GPU_ONLY);

        m_uniform_buffers.reserve(kat::MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < kat::MAX_FRAMES_IN_FLIGHT; i++) {
            m_uniform_buffers.push_back(m_context->gpu_allocator()->create_buffer(sizeof(UniformBuffer), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU));
        }
    }

    void Game::update(float dt) {

        constexpr float     SPEED      = 1.0f;
        constexpr float     TURN_SPEED = 1.0f;
        constexpr glm::vec3 UP         = {0.0f, -1.0f, 0.0f};

        glm::vec3 up           = glm::inverse(m_rot) * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
        glm::vec3 forward      = glm::inverse(m_rot) * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
        glm::vec3 right        = glm::inverse(m_rot) * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 flat_forward = glm::normalize(glm::vec3(forward.x, 0, forward.z));

        if (window()->get_key(GLFW_KEY_A)) {
            m_pos += dt * right * SPEED;
        }

        if (window()->get_key(GLFW_KEY_D)) {
            m_pos -= dt * right * SPEED;
        }

        if (window()->get_key(GLFW_KEY_Q)) {
            m_pos += dt * UP * SPEED;
        }

        if (window()->get_key(GLFW_KEY_E)) {
            m_pos -= dt * UP * SPEED;
        }

        if (window()->get_key(GLFW_KEY_S)) {
            m_pos += dt * flat_forward * SPEED;
        }

        if (window()->get_key(GLFW_KEY_W)) {
            m_pos -= dt * flat_forward * SPEED;
        }

        if (window()->get_key(GLFW_KEY_LEFT)) {
            m_rot = glm::rotate(m_rot, TURN_SPEED * dt, UP);
        }

        if (window()->get_key(GLFW_KEY_RIGHT)) {
            m_rot = glm::rotate(m_rot, -TURN_SPEED * dt, UP);
        }

        if (window()->get_key(GLFW_KEY_UP)) {
            m_rot = glm::rotate(m_rot, TURN_SPEED * dt, right);
        }

        if (window()->get_key(GLFW_KEY_DOWN)) {
            m_rot = glm::rotate(m_rot, -TURN_SPEED * dt, right);
        }

        if (window()->get_key(GLFW_KEY_R)) {
            m_rot = glm::identity<glm::fquat>();
            m_pos = {0.0f, 0.0f, -2.0f};
        }
    }

    void Game::render(const kat::FrameInfo &frame_info, float dt) {
        auto cmd = m_command_buffers[m_context->current_frame()];

        PushConstants pc = {glm::identity<glm::mat4>()};

        update_ubo();

        cmd.reset();
        cmd.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        kat::RenderPass::BeginInfo begin_info{};
        begin_info.render_area  = m_context->full_render_area();
        begin_info.clear_values = {kat::BLACK};
        begin_info.framebuffer  = m_framebuffers[frame_info.image_index];

        m_render_pass->begin(cmd, begin_info);
        m_graphics_pipeline->bind(cmd);

        cmd.bindIndexBuffer(m_index_buffer->handle(), 0, vk::IndexType::eUint32);

        const vk::Buffer         buf = m_vertex_buffer->handle();
        constexpr vk::DeviceSize off = 0;
        cmd.bindVertexBuffers(0, buf, off);

        std::vector<vk::DescriptorSet> sets = {m_descriptor_sets[m_context->current_frame()]};
        m_pipeline_layout->bind_descriptor_sets(cmd, vk::PipelineBindPoint::eGraphics, 0, sets, {});
        cmd.pushConstants<PushConstants>(m_pipeline_layout->handle(), vk::ShaderStageFlagBits::eAllGraphics, 0, pc);

        cmd.drawIndexed(36, 1, 0, 0, 0);

        m_render_pass->end(cmd);

        cmd.end();

        std::vector<vk::PipelineStageFlags> wait_stages{vk::PipelineStageFlagBits::eColorAttachmentOutput};

        vk::SubmitInfo si{};
        si.setCommandBuffers(cmd);
        si.setWaitSemaphores(frame_info.image_available_semaphore);
        si.setSignalSemaphores(frame_info.render_finished_semaphore);
        si.setWaitDstStageMask(wait_stages);

        m_context->graphics_queue().submit(si, frame_info.in_flight_fence);
    }

    void Game::update_ubo() {
        float aspect = m_window->aspect();

        glm::mat4 view = glm::mat4(m_rot) * glm::translate(glm::identity<glm::mat4>(), m_pos);
        glm::mat4 projection = glm::perspectiveFov(glm::radians(90.0f), 2.0f, 2.0f * aspect, 0.1f, 100.0f);

        glm::mat4 pv_matrix = projection * view;


        UniformBuffer ub   = {pv_matrix, kat::WHITE, kat::WHITE, glm::vec4{0.5f, -2.0f, 1.5f, 1.0f}, glm::vec4(-m_pos, 1.0f), 0.05f, 0.5f};
        auto          ubuf = m_uniform_buffers[m_context->current_frame()];
        ubuf->map_and_write_obj(ub, 0);
    }

} // namespace game
