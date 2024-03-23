#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

#include <iostream>
#include <memory>

#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

#include "kat/graphics/context.hpp"
#include "kat/graphics/graphics_pipeline.hpp"
#include "kat/graphics/render_pass.hpp"
#include "kat/graphics/shader_cache.hpp"
#include "kat/graphics/window.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <atomic>
#include <thread>

std::atomic_bool running = true;
float aspect;
glm::vec3 pos = {0.0f, 0.0f, -2.0f};
glm::fquat rot = glm::identity<glm::fquat>();
float fps = 1.0f / 60.0f;

struct Vertex {
    glm::vec3 position;
    alignas(16) kat::color color;
    glm::vec3 normal;
};

struct PushConstants {
    glm::mat4 model;
};

struct UniformBuffer {
    glm::mat4 view_projection;

    alignas(16) kat::color ambient_light_color;
    alignas(16) kat::color light_color;

    glm::vec4 light_pos;
    glm::vec4 view_pos;

    float     ambient_stength;
    float     specular_strength;
};

class RenderInfo {
  public:
    std::shared_ptr<kat::Context> context;
    std::vector<vk::Framebuffer>  framebuffers;

    std::shared_ptr<kat::RenderPass>          render_pass;
    std::shared_ptr<kat::DescriptorSetLayout> descriptor_set_layout;
    std::shared_ptr<kat::PipelineLayout>      pipeline_layout;
    std::shared_ptr<kat::GraphicsPipeline>    graphics_pipeline;
    std::shared_ptr<kat::DescriptorPool>      descriptor_pool;

    vk::CommandPool                                          command_pool;
    std::array<vk::CommandBuffer, kat::MAX_FRAMES_IN_FLIGHT> command_buffers;

    std::shared_ptr<kat::Buffer> index_buffer;
    std::shared_ptr<kat::Buffer> vertex_buffer;

    std::vector<std::shared_ptr<kat::Buffer>> uniform_buffers;
    std::vector<vk::DescriptorSet>            descriptor_sets;

    explicit RenderInfo(const std::shared_ptr<kat::Context> &context_) : context(context_) {
        command_pool    = context->create_command_pool_raw<kat::QueueType::GRAPHICS>();
        command_buffers = context->allocate_command_buffers_raw<kat::MAX_FRAMES_IN_FLIGHT>(command_pool);

        create_render_pass();
        create_framebuffers();
        create_uniform_buffer();
        create_pipeline_layout();
        create_graphics_pipeline();
        create_vertex_buffer();
    };

    ~RenderInfo() = default;

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
        {
            kat::DescriptorSetLayout::Description desc{};
            desc.bindings = {vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eAllGraphics, {})};

            descriptor_set_layout = std::make_shared<kat::DescriptorSetLayout>(context, desc);
        }

        {
            kat::PipelineLayout::Description desc{};

            desc.push_constant_ranges   = {vk::PushConstantRange(vk::ShaderStageFlagBits::eAllGraphics, 0, sizeof(PushConstants))};
            desc.descriptor_set_layouts = {descriptor_set_layout};

            pipeline_layout = std::make_shared<kat::PipelineLayout>(context, desc);
        }

        {
            kat::DescriptorPool::Description desc{};
            desc.max_sets   = kat::MAX_FRAMES_IN_FLIGHT;
            desc.pool_sizes = {vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, kat::MAX_FRAMES_IN_FLIGHT)};
            descriptor_pool = std::make_shared<kat::DescriptorPool>(context, desc);
        }

        descriptor_sets = descriptor_pool->allocate_sets(descriptor_set_layout, kat::MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < kat::MAX_FRAMES_IN_FLIGHT; i++) {
            vk::DescriptorBufferInfo dbi{};
            dbi.buffer = uniform_buffers[i]->handle();
            dbi.offset = 0;
            dbi.range  = sizeof(UniformBuffer);

            vk::WriteDescriptorSet write{};
            write.dstSet          = descriptor_sets[i];
            write.dstBinding      = 0;
            write.dstArrayElement = 0;
            write.descriptorType  = vk::DescriptorType::eUniformBuffer;
            write.setBufferInfo(dbi);

            context->device().updateDescriptorSets(write, {});
        }
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
                                                          sizeof(Vertex),
                                                          {
                                                              kat::VertexAttribute{0, vk::Format::eR32G32B32Sfloat, 0},
                                                              kat::VertexAttribute{1, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, color)},
                                                              kat::VertexAttribute{2, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)},
                                                          },
                                                          vk::VertexInputRate::eVertex}};

        desc.layout      = pipeline_layout;
        desc.render_pass = render_pass;
        desc.subpass     = 0;

        graphics_pipeline = std::make_shared<kat::GraphicsPipeline>(context, desc);
    }

    void create_vertex_buffer() {
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

        vertex_buffer = context->gpu_allocator()->init_buffer(vertices, vk::BufferUsageFlagBits::eVertexBuffer, VMA_MEMORY_USAGE_GPU_ONLY);

        const std::vector<uint32_t> indices = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17,
                                               18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35};

        index_buffer = context->gpu_allocator()->init_buffer(indices, vk::BufferUsageFlagBits::eIndexBuffer, VMA_MEMORY_USAGE_GPU_ONLY);
    }

    void create_uniform_buffer() {
        uniform_buffers.reserve(kat::MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < kat::MAX_FRAMES_IN_FLIGHT; i++) {
            uniform_buffers.push_back(context->gpu_allocator()->create_buffer(sizeof(UniformBuffer), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU));
        }
    }

    float this_frame = glfwGetTime();
    float last_frame = this_frame - 0.01667;
    float delta      = 0.01667;

    void update_ubo() {
        glm::mat4 view = glm::mat4(rot) * glm::translate(glm::identity<glm::mat4>(), pos);

        glm::mat4 projection = glm::perspectiveFov(glm::radians(90.0f), 2.0f, 2.0f * aspect, 0.1f, 100.0f);

        glm::mat4 pv_matrix = projection * view;

        UniformBuffer ub   = {pv_matrix, kat::WHITE, kat::WHITE, glm::vec4{0.5f, -2.0f, 1.5f, 1.0f}, glm::vec4(-pos, 1.0f), 0.05f, 0.5f};
        auto          ubuf = uniform_buffers[context->current_frame()];
        ubuf->map_and_write_obj(ub, 0);
    }

    void render(const kat::FrameInfo &frame_info) {
        auto cmd = command_buffers[context->current_frame()];


        PushConstants pc = {glm::identity<glm::mat4>()};

        update_ubo();


        cmd.reset();
        cmd.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        kat::RenderPass::BeginInfo begin_info{};
        begin_info.render_area  = context->full_render_area();
        begin_info.clear_values = {kat::BLACK};
        begin_info.framebuffer  = framebuffers[frame_info.image_index];

        render_pass->begin(cmd, begin_info);
        graphics_pipeline->bind(cmd);

        cmd.bindIndexBuffer(index_buffer->handle(), 0, vk::IndexType::eUint32);

        const vk::Buffer         buf = vertex_buffer->handle();
        constexpr vk::DeviceSize off = 0;
        cmd.bindVertexBuffers(0, buf, off);

        std::vector<vk::DescriptorSet> sets = {descriptor_sets[context->current_frame()]};
        pipeline_layout->bind_descriptor_sets(cmd, vk::PipelineBindPoint::eGraphics, 0, sets, {});
        cmd.pushConstants<PushConstants>(pipeline_layout->handle(), vk::ShaderStageFlagBits::eAllGraphics, 0, pc);

        cmd.drawIndexed(36, 1, 0, 0, 0);

        render_pass->end(cmd);

        cmd.end();

        std::vector<vk::PipelineStageFlags> wait_stages{vk::PipelineStageFlagBits::eColorAttachmentOutput};

        vk::SubmitInfo si{};
        si.setCommandBuffers(cmd);
        si.setWaitSemaphores(frame_info.image_available_semaphore);
        si.setSignalSemaphores(frame_info.render_finished_semaphore);
        si.setWaitDstStageMask(wait_stages);

        context->graphics_queue().submit(si, frame_info.in_flight_fence);

        last_frame = this_frame;
        this_frame = glfwGetTime();
        delta      = this_frame - last_frame;

        fps = 1.0f / delta;
    }
};

void run() {
    const auto window = std::make_unique<kat::Window>(kat::WindowSettings{.title = "Window", .fullscreen = true});
    
    aspect = static_cast<float>(window->size().y) / static_cast<float>(window->size().x);

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

    float this_update = glfwGetTime();
    float delta       = 0.01667;
    float last_update = this_update - delta;

    constexpr float     SPEED      = 1.0f;
    constexpr float     TURN_SPEED = 1.0f;
    constexpr glm::vec3 UP         = {0.0f, -1.0f, 0.0f};

    while (window->is_open()) {
        kat::Window::poll();

        glm::vec3 up      = glm::inverse(rot) * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
        glm::vec3 forward = glm::inverse(rot) * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
        glm::vec3 right   = glm::inverse(rot) * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);

        glm::vec3 flat_forward = glm::normalize(glm::vec3(forward.x, 0, forward.z));

        if (window->get_key(GLFW_KEY_A)) {
            pos += delta * right * SPEED;
        }

        if (window->get_key(GLFW_KEY_D)) {
            pos -= delta * right * SPEED;
        }

        if (window->get_key(GLFW_KEY_Q)) {
            pos += delta * UP * SPEED;
        }

        if (window->get_key(GLFW_KEY_E)) {
            pos -= delta * UP * SPEED;
        }

        if (window->get_key(GLFW_KEY_S)) {
            pos += delta * flat_forward * SPEED;
        }

        if (window->get_key(GLFW_KEY_W)) {
            pos -= delta * flat_forward * SPEED;
        }

        if (window->get_key(GLFW_KEY_LEFT)) {
            rot = glm::rotate(rot, TURN_SPEED * delta, UP);
        }

        if (window->get_key(GLFW_KEY_RIGHT)) {
            rot = glm::rotate(rot, -TURN_SPEED * delta, UP);
        }

        if (window->get_key(GLFW_KEY_UP)) {
            rot = glm::rotate(rot, TURN_SPEED * delta, right);
        }

        if (window->get_key(GLFW_KEY_DOWN)) {
            rot = glm::rotate(rot, -TURN_SPEED * delta, right);
        }

        if (window->get_key(GLFW_KEY_R)) {
            rot = glm::identity<glm::fquat>();
            pos = {0.0f, 0.0f, -2.0f};
        }


        last_update = this_update;
        this_update = glfwGetTime();
        delta       = this_update - last_update;

        std::cout << "FPS: " << fps << '\n';
    }

    running = false;

    render_thread.join();
}

int main() {
    int ec = EXIT_SUCCESS;
    try {
        glfwInit();
        run();
    } catch (const std::exception &e) {
        std::cerr << "Fatally crashed: " << e.what() << std::endl;
        std::cerr << "Fatal error encountered. Exiting." << std::endl;
        ec = EXIT_FAILURE;
    }

    glfwTerminate();

    return ec;
}
