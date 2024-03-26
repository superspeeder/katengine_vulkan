#include "app.hpp"

#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

namespace kat {
    App::App(const WindowSettings &window_settings, const ContextSettings &context_settings, const std::filesystem::path& resources_dir) : m_resources_dir(resources_dir) {
        glfwInit();
        m_window = std::make_unique<Window>(window_settings);

        m_context = kat::Context::init(m_window, context_settings);

        m_this_frame   = static_cast<float>(glfwGetTime());
        m_this_update  = static_cast<float>(glfwGetTime());
        m_render_delta = 1.0 / 60.0;
        m_update_delta = 1.0 / 60.0;
        m_last_frame   = m_this_frame - m_render_delta;
        m_last_update  = m_this_frame - m_render_delta;
    }

    void App::launch() {
        m_render_thread = std::jthread([this]() {
            while (m_is_running) {
                auto frame_info = m_context->acquire_next_frame();

                render(frame_info, m_render_delta);

                m_context->present();

                m_last_frame   = m_this_frame;
                m_this_frame   = static_cast<float>(glfwGetTime());
                m_render_delta = m_this_frame - m_last_frame;
            }
        });

        while (m_window->is_open()) {
            kat::Window::poll();

            update(m_update_delta);

            m_last_update  = m_this_update;
            m_this_update  = static_cast<float>(glfwGetTime());
            m_update_delta = m_this_update - m_last_update;
        }

        m_is_running = false;
        m_render_thread.join();

        m_context->device().waitIdle();
    }

    ImGuiResources::ImGuiResources(const std::unique_ptr<Window> &window, const std::shared_ptr<Context> &context, vk::RenderPass render_pass) {
        {
            kat::DescriptorPool::Description desc{};
            desc.max_sets   = 1000;
            desc.pool_sizes = {
                vk::DescriptorPoolSize(vk::DescriptorType::eSampler, 1000),
                vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1000),
                vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, 1000),
                vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 1000),
                vk::DescriptorPoolSize(vk::DescriptorType::eUniformTexelBuffer, 1000),
                vk::DescriptorPoolSize(vk::DescriptorType::eStorageTexelBuffer, 1000),
                vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1000),
                vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1000),
                vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1000),
                vk::DescriptorPoolSize(vk::DescriptorType::eStorageBufferDynamic, 1000),
                vk::DescriptorPoolSize(vk::DescriptorType::eInputAttachment, 1000),
            };

            descriptor_pool = std::make_shared<kat::DescriptorPool>(context, desc);
        }

        ImGui::CreateContext();

        ImGui_ImplGlfw_InitForVulkan(window->handle(), true);

        ImGui_ImplVulkan_InitInfo init_info{};
        init_info.Instance = context->instance();
        init_info.PhysicalDevice = context->physical_device();
        init_info.Device = context->device();
        init_info.Queue = context->graphics_queue();
        init_info.MinImageCount = context->vkb_swapchain().requested_min_image_count;
        init_info.ImageCount = context->vkb_swapchain().image_count;
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.DescriptorPool = descriptor_pool->handle();

        ImGui_ImplVulkan_Init(&init_info, render_pass);

        ImGui_ImplVulkan_CreateFontsTexture();

    }
} // namespace kat
