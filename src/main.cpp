#include <iostream>
#include <memory>

#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

#include "kat/graphics/context.hpp"
#include "kat/graphics/graphics_pipeline.hpp"
#include "kat/graphics/render_pass.hpp"
#include "kat/graphics/shader_cache.hpp"
#include "kat/graphics/window.hpp"

#include <atomic>
#include <thread>

std::atomic_bool running = true;

struct Vertex {
    glm::vec3 position;
    glm::vec4 color;
};

class RenderInfo {
  public:
    std::shared_ptr<kat::Context> context;
    std::vector<vk::Framebuffer>  framebuffers;

    std::shared_ptr<kat::RenderPass>       render_pass;
    std::shared_ptr<kat::PipelineLayout>   pipeline_layout;
    std::shared_ptr<kat::GraphicsPipeline> graphics_pipeline;


    vk::CommandPool                                          command_pool;
    std::array<vk::CommandBuffer, kat::MAX_FRAMES_IN_FLIGHT> command_buffers;

    std::shared_ptr<kat::Buffer> vertex_buffer;

    explicit RenderInfo(const std::shared_ptr<kat::Context> &context_) : context(context_) {
        command_pool    = context->create_command_pool_raw<kat::QueueType::GRAPHICS>();
        command_buffers = context->allocate_command_buffers_raw<kat::MAX_FRAMES_IN_FLIGHT>(command_pool);

        create_render_pass();
        create_framebuffers();
        create_pipeline_layout();
        create_graphics_pipeline();
        create_vertex_buffer();
    };

    ~RenderInfo(){};

    void create_render_pass() {
        kat::RenderPass::Description desc{};

        desc.attachments.push_back(kat::AttachmentLayout{
            .format       = context->swapchain_format(),
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

        render_pass = std::make_shared<kat::RenderPass>(context, desc);
    };

    void create_framebuffers() { framebuffers = render_pass->create_framebuffers(context->swapchain_image_views(), context->swapchain_extent()); };

    void create_pipeline_layout() {
        kat::PipelineLayout::Description desc{};
        pipeline_layout = std::make_shared<kat::PipelineLayout>(context, desc);
    }

    void create_graphics_pipeline() {
        kat::GraphicsPipeline::Description desc{};

        desc.add_shader("shaders/main.vert.spv", vk::ShaderStageFlagBits::eVertex);
        desc.add_shader("shaders/main.frag.spv", vk::ShaderStageFlagBits::eFragment);

        desc.blend_state.blend_attachments = {
            kat::STANDARD_BLEND_STATE,
        };

        desc.viewports.push_back(context->full_viewport());
        desc.scissors.push_back(context->full_render_area());

        desc.vertex_layout.bindings = {kat::VertexBinding{0,
                                                          sizeof(float) * 7,
                                                          {
                                                              kat::VertexAttribute{0, vk::Format::eR32G32B32Sfloat, 0},
                                                              kat::VertexAttribute{1, vk::Format::eR32G32B32A32Sfloat, 3 * sizeof(float)},
                                                          },
                                                          vk::VertexInputRate::eVertex}};

        desc.layout      = pipeline_layout;
        desc.render_pass = render_pass;
        desc.subpass     = 0;

        graphics_pipeline = std::make_shared<kat::GraphicsPipeline>(context, desc);
    }

    void create_vertex_buffer() {
        const std::vector<Vertex> vertices = {
            {glm::vec3(-0.5f, 0.5f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)},
            {glm::vec3(0.0f, -0.5f, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)},
            {glm::vec3(0.5f, 0.5f, 0.0f), glm::vec4(1.0f, 0.0f, 1.0f, 1.0f)},
        };

        vertex_buffer = context->gpu_allocator()->init_buffer(vertices, vk::BufferUsageFlagBits::eVertexBuffer, VMA_MEMORY_USAGE_GPU_ONLY);
    }

    void render(const kat::FrameInfo &frame_info) {
        auto cmd = command_buffers[context->current_frame()];

        cmd.reset();
        cmd.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        const auto v = static_cast<float>((1 + sin(glfwGetTime())) * 0.5);

        kat::RenderPass::BeginInfo begin_info{};
        begin_info.render_area  = context->full_render_area();
        begin_info.clear_values = {kat::color(v, v, v)};
        begin_info.framebuffer  = framebuffers[frame_info.image_index];

        render_pass->begin(cmd, begin_info);
        graphics_pipeline->bind(cmd);

        const vk::Buffer         buf = vertex_buffer->handle();
        constexpr vk::DeviceSize off = 0;
        cmd.bindVertexBuffers(0, buf, off);

        cmd.draw(3, 1, 0, 0);

        render_pass->end(cmd);

        cmd.end();

        std::vector<vk::PipelineStageFlags> wait_stages{vk::PipelineStageFlagBits::eColorAttachmentOutput};

        vk::SubmitInfo si{};
        si.setCommandBuffers(cmd);
        si.setWaitSemaphores(frame_info.image_available_semaphore);
        si.setSignalSemaphores(frame_info.render_finished_semaphore);
        si.setWaitDstStageMask(wait_stages);

        context->graphics_queue().submit(si, frame_info.in_flight_fence);
    }
};

void run() {
    const auto window = std::make_unique<kat::Window>(kat::WindowSettings{.size = {800, 600}, .pos = {GLFW_ANY_POSITION, GLFW_ANY_POSITION}, .title = "Window"});

    auto context = kat::Context::init(window, kat::ContextSettings{});

    auto render_thread = std::jthread([&]() {
        const auto render_info = std::make_unique<RenderInfo>(context);

        while (running) {
            auto frame_info = context->acquire_next_frame();

            render_info->render(frame_info);

            context->present();
        }

        context->device().waitIdle();
    });

    while (window->is_open()) {
        kat::Window::poll();
    }

    running = false;

    render_thread.join();
}

int main() {
    int ec = EXIT_SUCCESS;
    try {
        glfwInit();
        run();
    } catch (kat::fatal_exc) {
        std::cerr << "Fatal error encountered. Exiting." << std::endl;
        ec = EXIT_FAILURE;
    }

    glfwTerminate();

    return ec;
}
