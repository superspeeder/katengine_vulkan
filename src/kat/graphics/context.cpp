#include "kat/graphics/context.hpp"

#include <GLFW/glfw3.h>
#include <set>

#include <iostream>

#include <algorithm>
#include <ranges>

#include "kat/graphics/shader_cache.hpp"

namespace kat {

    uint32_t vkapiver(const Version &ver) {
        return VK_MAKE_API_VERSION(0, ver.major, ver.minor, ver.patch);
    }

    Context::Context(const std::unique_ptr<Window> &window, const ContextSettings &settings) {
        vkb::InstanceBuilder instance_builder;

        auto inst_ret = instance_builder.require_api_version(VK_API_VERSION_1_3)
                            .set_app_name(settings.app_name.c_str())
                            .set_engine_name("KatEngine")
                            .set_app_version(vkapiver(settings.app_version))
                            .set_engine_version(VK_MAKE_API_VERSION(0, 0, 1, 0))
                            .request_validation_layers()
                            .use_default_debug_messenger()
                            .build();

        if (!inst_ret) {
            std::cerr << "Failed to create vk instance. Error: " << inst_ret.error().message() << std::endl;
            throw kat::fatal;
        }

        m_inst     = inst_ret.value();
        m_instance = m_inst.instance;

        m_surface = window->create_surface(m_instance);

        vkb::PhysicalDeviceSelector selector{m_inst};

        vk::PhysicalDeviceFeatures         features{};
        vk::PhysicalDeviceVulkan11Features features11{};
        vk::PhysicalDeviceVulkan12Features features12{};
        vk::PhysicalDeviceVulkan13Features features13{};

        features.fillModeNonSolid   = true;
        features.geometryShader     = true;
        features.tessellationShader = true;
        features.wideLines          = true;
        features.largePoints        = true;

        features11.variablePointers              = true;
        features11.variablePointersStorageBuffer = true;
        features12.timelineSemaphore             = true;
        features12.uniformBufferStandardLayout   = true;
        features13.dynamicRendering              = true;


        auto phys_ret = selector.set_surface(m_surface)
                            .require_present()
                            .set_minimum_version(1, 3)
                            .set_required_features(features)
                            .set_required_features_11(features11)
                            .set_required_features_12(features12)
                            .set_required_features_13(features13)
                            .set_desired_version(1, 3)
                            .select();

        if (!phys_ret) {
            std::cerr << "Failed to select physical device. Error: " << phys_ret.error().message() << std::endl;
            throw kat::fatal;
        }


        m_phys            = phys_ret.value();
        m_physical_device = m_phys.physical_device;

        vkb::DeviceBuilder device_builder{m_phys};

        auto dev_ret = device_builder.build();

        if (!dev_ret) {
            std::cerr << "Failed to create logical device. Error: " << dev_ret.error().message() << std::endl;
            throw kat::fatal;
        }

        m_dev    = dev_ret.value();
        m_device = m_dev.device;

        auto gqr = m_dev.get_queue(vkb::QueueType::graphics);
        if (!gqr) {
            std::cerr << "Failed to get graphics queue. Error: " << gqr.error().message();
            throw kat::fatal;
        }
        m_graphics_queue = gqr.value();

        auto pqr = m_dev.get_queue(vkb::QueueType::present);
        if (!pqr) {
            std::cerr << "Failed to get present queue. Error: " << pqr.error().message();
            throw kat::fatal;
        }
        m_present_queue = pqr.value();

        auto tqr = m_dev.get_queue(vkb::QueueType::transfer);
        if (!tqr) {
            std::cerr << "Failed to get transfer queue. Error: " << tqr.error().message();
            throw kat::fatal;
        }
        m_transfer_queue = tqr.value();

        auto cqr = m_dev.get_queue(vkb::QueueType::compute);
        if (!cqr) {
            std::cerr << "Failed to get compute queue. Error: " << cqr.error().message();
            throw kat::fatal;
        }
        m_compute_queue = cqr.value();

        m_graphics_family = m_dev.get_queue_index(vkb::QueueType::graphics).value();
        m_present_family  = m_dev.get_queue_index(vkb::QueueType::present).value();
        m_transfer_family = m_dev.get_queue_index(vkb::QueueType::transfer).value();
        m_compute_family  = m_dev.get_queue_index(vkb::QueueType::compute).value();

        create_swapchain();

        m_image_available_semaphores = create_semaphores<MAX_FRAMES_IN_FLIGHT>();
        m_render_finished_semaphores = create_semaphores<MAX_FRAMES_IN_FLIGHT>();
        m_in_flight_fences           = create_fences_signaled<MAX_FRAMES_IN_FLIGHT>();
    }

// init things that need shared_from_this()
    void Context::init() {
        m_shader_cache = std::make_unique<kat::ShaderCache>(shared_from_this());
    }

    Context::~Context() {}

    void Context::create_swapchain() {
        vkb::SwapchainBuilder swb{m_dev};

        vk::ImageUsageFlags iuf = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;

        auto swc_ret = swb.set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
                           .add_fallback_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR)
                           .add_fallback_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                           .set_old_swapchain(m_swc)
                           .set_image_usage_flags(static_cast<VkImageUsageFlags>(iuf))
                           .set_clipped(true)
                           .build();

        if (!swc_ret) {
            std::cerr << "Failed to create swapchain. Error: " << swc_ret.error().message() << std::endl;
            throw kat::fatal;
        }


        if (m_swc.swapchain != VK_NULL_HANDLE)
            vkb::destroy_swapchain(m_swc);

        m_swc       = swc_ret.value();
        m_swapchain = m_swc.swapchain;

        auto swci    = m_swc.get_images().value();
        m_swc_images.reserve(swci.size());
        for (const auto& i : swci) {
            m_swc_images.push_back(i);
        }

        auto swciv    = m_swc.get_image_views().value();
        m_swc_image_views.reserve(swci.size());
        for (const auto& iv : swciv) {
            m_swc_image_views.push_back(iv);
        }
    }

    FrameInfo Context::acquire_next_frame() {
        auto fence = m_in_flight_fences[m_current_frame];
        wait_for_fences({fence});

        auto next_image_index_res = m_device.acquireNextImageKHR(m_swapchain, UINT64_MAX, m_image_available_semaphores[m_current_frame]);

        m_current_image_index = next_image_index_res.value;

        reset_fences({fence});

        return {
            m_image_available_semaphores[m_current_frame],
            m_render_finished_semaphores[m_current_frame],
            m_swc_images.at(m_current_image_index),
            m_swc_image_views.at(m_current_image_index),
            fence,
            m_current_image_index,
        };
    }

    vk::Semaphore Context::create_semaphore() {
        return m_device.createSemaphore(vk::SemaphoreCreateInfo());
    }

    vk::Fence Context::create_fence(bool signaled) {
        if (signaled)
            return create_fence_signaled();
        return create_fence();
    }

    vk::Fence Context::create_fence() {
        return m_device.createFence(vk::FenceCreateInfo());
    }

    vk::Fence Context::create_fence_signaled() {
        return m_device.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
    }

    bool Context::wait_for_fences(const std::vector<vk::Fence> &fences) {
        auto res = m_device.waitForFences(fences, true, UINT64_MAX);

        return res == vk::Result::eSuccess;
    }

    void Context::reset_fences(const std::vector<vk::Fence> &fences) {
        m_device.resetFences(fences);
    }

    vk::CommandPool Context::create_command_pool_raw(uint32_t family, bool allow_individual_reset) const {
        return m_device.createCommandPool(vk::CommandPoolCreateInfo(
            allow_individual_reset ? vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer) : vk::CommandPoolCreateFlags(0), family));
    }

    void Context::present() {
        vk::PresentInfoKHR pi{};
        pi.setSwapchains(m_swapchain);
        pi.setImageIndices(m_current_image_index);
        pi.setWaitSemaphores(m_render_finished_semaphores[m_current_frame]);

        auto res = m_present_queue.presentKHR(pi);
        if (res != vk::Result::eSuccess) {
            std::cerr << "Warning: Present returned result " << vk::to_string(res) << std::endl;
        }

        m_current_frame = (m_current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
    }


} // namespace kat
