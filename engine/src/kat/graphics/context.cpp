#include "kat/graphics/context.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <GLFW/glfw3.h>
#include <set>

#include <iostream>

#include <algorithm>
#include <ranges>

#include "context.hpp"
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
            throw fatal_exc{};
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
        features.samplerAnisotropy  = true;

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
                            // .set_desired_version(1, 3)
                            .select();

        if (!phys_ret) {
            std::cerr << "Failed to select physical device. Error: " << phys_ret.error().message() << std::endl;
            throw fatal_exc{};
        }


        m_phys            = phys_ret.value();
        m_physical_device = m_phys.physical_device;

        vkb::DeviceBuilder device_builder{m_phys};

        auto dev_ret = device_builder.build();

        if (!dev_ret) {
            std::cerr << "Failed to create logical device. Error: " << dev_ret.error().message() << std::endl;
            throw fatal_exc{};
        }

        m_dev    = dev_ret.value();
        m_device = m_dev.device;

        auto gqr = m_dev.get_queue(vkb::QueueType::graphics);
        if (!gqr) {
            std::cerr << "Failed to get graphics queue. Error: " << gqr.error().message();
            throw fatal_exc{};
        }
        m_graphics_queue = gqr.value();

        auto pqr = m_dev.get_queue(vkb::QueueType::present);
        if (!pqr) {
            std::cerr << "Failed to get present queue. Error: " << pqr.error().message();
            throw fatal_exc{};
        }
        m_present_queue = pqr.value();

        auto tqr = m_dev.get_queue(vkb::QueueType::transfer);
        if (!tqr) {
            std::cerr << "Failed to get transfer queue. Error: " << tqr.error().message();
            throw fatal_exc{};
        }
        m_transfer_queue = tqr.value();

        auto cqr = m_dev.get_queue(vkb::QueueType::compute);
        if (!cqr) {
            std::cerr << "Failed to get compute queue. Error: " << cqr.error().message();
            throw fatal_exc{};
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

        m_single_time_gt_pool = create_command_pool_raw(m_graphics_family, true);
    }

    // init things that need shared_from_this()
    void Context::init() {
        m_shader_cache  = std::make_unique<ShaderCache>(shared_from_this());
        m_gpu_allocator = std::make_unique<GpuAllocator>(shared_from_this());
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
            throw fatal_exc{};
        }


        if (m_swc.swapchain != VK_NULL_HANDLE)
            vkb::destroy_swapchain(m_swc);

        m_swc       = swc_ret.value();
        m_swapchain = m_swc.swapchain;

        auto swci = m_swc.get_images().value();
        m_swc_images.reserve(swci.size());
        for (const auto &i : swci) {
            m_swc_images.push_back(i);
        }

        auto swciv = m_swc.get_image_views().value();
        m_swc_image_views.reserve(swci.size());
        for (const auto &iv : swciv) {
            m_swc_image_views.push_back(iv);
        }
    }

    FrameInfo Context::acquire_next_frame() {
        auto fence = m_in_flight_fences[m_current_frame];

        // ReSharper disable once CppExpressionWithoutSideEffects
        wait_for_fences({fence});

        const auto next_image_index_res = m_device.acquireNextImageKHR(m_swapchain, UINT64_MAX, m_image_available_semaphores[m_current_frame]);

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

    vk::Semaphore Context::create_semaphore() const {
        return m_device.createSemaphore(vk::SemaphoreCreateInfo());
    }

    vk::Fence Context::create_fence(bool signaled) const {
        if (signaled)
            return create_fence_signaled();
        return create_fence();
    }

    vk::Fence Context::create_fence() const {
        return m_device.createFence(vk::FenceCreateInfo());
    }

    vk::Fence Context::create_fence_signaled() const {
        return m_device.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
    }

    bool Context::wait_for_fences(const std::vector<vk::Fence> &fences) const {
        return m_device.waitForFences(fences, true, UINT64_MAX) == vk::Result::eSuccess;
    }

    void Context::reset_fences(const std::vector<vk::Fence> &fences) const {
        m_device.resetFences(fences);
    }

    vk::Queue Context::get_queue(const QueueType &queue_type) const {
        switch (queue_type) {
        case QueueType::GRAPHICS:
            return graphics_queue();
        case QueueType::PRESENT:
            return present_queue();
        case QueueType::TRANSFER:
            return transfer_queue();
        case QueueType::COMPUTE:
            return compute_queue();
        }

        throw std::runtime_error("Bad value for kat::QueueType");
    }

    uint32_t Context::get_queue_family(const QueueType &queue_type) const {
        switch (queue_type) {
        case QueueType::GRAPHICS:
            return graphics_family();
        case QueueType::PRESENT:
            return present_family();
        case QueueType::TRANSFER:
            return transfer_family();
        case QueueType::COMPUTE:
            return compute_family();
        }

        throw std::runtime_error("Bad value for kat::QueueType");
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

        if (const auto res = m_present_queue.presentKHR(pi); res != vk::Result::eSuccess) {
            std::cerr << "Warning: Present returned result " << vk::to_string(res) << std::endl;
        }

        m_current_frame = (m_current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    vk::CommandBuffer Context::begin_single_time_commands() const {
        const auto cmd = allocate_command_buffers_raw<1>(m_single_time_gt_pool)[0];
        cmd.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        return cmd;
    }

    void Context::end_single_time_commands(const vk::CommandBuffer &cmd) const {
        cmd.end();

        auto fence = create_fence();

        vk::SubmitInfo si{};
        si.setCommandBuffers(cmd);

        m_graphics_queue.submit(si, fence);

        wait_for_fences({fence}); // better than a queue wait idle since this actually lets other things happen (later we can offload the cleanup to a different thread).

        m_device.freeCommandBuffers(m_single_time_gt_pool, cmd);
        m_device.destroy(fence);
    }

    void Context::single_time_commands(const std::function<void(const vk::CommandBuffer &)> &f) const {
        auto cmd = begin_single_time_commands();
        f(cmd);
        end_single_time_commands(cmd);
    }

    Buffer::Buffer(const std::shared_ptr<Context> &context, vk::Buffer buffer, VmaAllocation allocation, VmaAllocationInfo allocation_info)
        : m_buffer(buffer), m_allocation(allocation), m_allocation_info(allocation_info), m_context(context) {}

    Buffer::~Buffer() {
        m_context->gpu_allocator()->free_buffer(m_buffer, m_allocation);
    }

    void Buffer::map_and_write(const void *data, const vk::DeviceSize &size, const vk::DeviceSize &offset) const {
        void *map = m_context->gpu_allocator()->map(m_allocation);
        // static cast allows for easy byte offsets.
        std::memcpy(static_cast<char *>(map) + offset, data, size);
        m_context->gpu_allocator()->unmap(m_allocation);
    }

    void Buffer::copy_from(const std::shared_ptr<Buffer> &other, const vk::DeviceSize &size, const vk::DeviceSize &src_offset, const vk::DeviceSize &dst_offset) const {
        const auto     cmd = m_context->begin_single_time_commands();
        vk::BufferCopy region{};
        region.size      = size;
        region.srcOffset = src_offset;
        region.dstOffset = dst_offset;
        cmd.copyBuffer(other->m_buffer, m_buffer, region);

        m_context->end_single_time_commands(cmd);
    }

    Image::Image(const std::shared_ptr<Context> &context, vk::Image image, VmaAllocation allocation, VmaAllocationInfo allocation_info)
        : m_image(image), m_allocation(allocation), m_allocation_info(allocation_info), m_context(context) {}

    Image::~Image() {
        m_context->gpu_allocator()->free_image(m_image, m_allocation);
    }

    GpuAllocator::GpuAllocator(const std::shared_ptr<Context> &context) : m_context(context) {
        VmaAllocatorCreateInfo ci{};
        ci.device           = context->device();
        ci.instance         = context->instance();
        ci.physicalDevice   = context->physical_device();
        ci.vulkanApiVersion = VK_API_VERSION_1_3;

        if (const auto res = static_cast<vk::Result>(vmaCreateAllocator(&ci, &m_allocator)); res != vk::Result::eSuccess) {
            std::cerr << "Error: Failed to create gpu allocator. Result: " << vk::to_string(res) << std::endl;
            throw fatal_exc{};
        }
    }

    GpuAllocator::~GpuAllocator() {
        vmaDestroyAllocator(m_allocator);
    }

    std::shared_ptr<Buffer> GpuAllocator::create_buffer(const vk::BufferCreateInfo &create_info, const VmaMemoryUsage &vma_memory_usage) const {
        const VkBufferCreateInfo ci = create_info;

        VmaAllocationCreateInfo ai{};
        ai.usage = vma_memory_usage;

        VkBuffer          buf;
        VmaAllocation     alloc;
        VmaAllocationInfo alloci{};
        if (const auto res = static_cast<vk::Result>(vmaCreateBuffer(m_allocator, &ci, &ai, &buf, &alloc, &alloci)); res != vk::Result::eSuccess) {
            std::cerr << "Failed to create buffer. Result: " << vk::to_string(res) << std::endl;
            throw fatal_exc{};
        }

        return std::make_shared<Buffer>(m_context, buf, alloc, alloci);
    }

    std::shared_ptr<Image> GpuAllocator::create_image(const vk::ImageCreateInfo &create_info, const VmaMemoryUsage &vma_memory_usage) const {
        const VkImageCreateInfo ci = create_info;

        VmaAllocationCreateInfo ai{};
        ai.usage = vma_memory_usage;

        VkImage           img;
        VmaAllocation     alloc;
        VmaAllocationInfo alloci{};
        if (const auto res = static_cast<vk::Result>(vmaCreateImage(m_allocator, &ci, &ai, &img, &alloc, &alloci)); res != vk::Result::eSuccess) {
            std::cerr << "Failed to create image. Result: " << vk::to_string(res) << std::endl;
            throw fatal_exc{};
        }

        return std::make_shared<Image>(m_context, img, alloc, alloci);
    }

    void GpuAllocator::free_buffer(const vk::Buffer &buffer, const VmaAllocation &allocation) const {
        vmaDestroyBuffer(m_allocator, buffer, allocation);
    }

    void GpuAllocator::free_image(const vk::Image &image, const VmaAllocation &allocation) const {
        vmaDestroyImage(m_allocator, image, allocation);
    }

    std::shared_ptr<Buffer> GpuAllocator::init_buffer(const void *data, const vk::DeviceSize &size, const vk::BufferUsageFlags usage,
                                                      const VmaMemoryUsage &vma_memory_usage) const {
        const auto staging_buffer = create_buffer(size, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
        const auto final_buffer   = create_buffer(size, vk::BufferUsageFlagBits::eTransferDst | usage, vma_memory_usage);

        staging_buffer->map_and_write(data, size, 0);
        final_buffer->copy_from(staging_buffer, size, 0, 0);

        return final_buffer;
    }

    void *GpuAllocator::map(const VmaAllocation &alloc) const {
        void *map;
        vmaMapMemory(m_allocator, alloc, &map);

        return map;
    }

    void GpuAllocator::unmap(const VmaAllocation &alloc) const {
        vmaUnmapMemory(m_allocator, alloc);
    }

    std::tuple<std::shared_ptr<Image>, vk::Format, vk::Extent2D> GpuAllocator::load_image(const std::filesystem::path &path) const {
        std::string path_ = path.string();

        int width, height;
        int components;

        unsigned char *data = stbi_load(path_.c_str(), &width, &height, &components, 0);

        vk::Format format = vk::Format::eR8G8B8A8Unorm;

        switch (components) {
        case 1:
            format = vk::Format::eR8Unorm;
            break;
        case 2:
            format = vk::Format::eR8G8Unorm;
            break;
        case 3:
            format = vk::Format::eR8G8B8Unorm;
            break;
        case 4:
            format = vk::Format::eR8G8B8A8Unorm;
            break;
        }

        auto image = init_image(width, height, components, format, data, vk::ImageUsageFlagBits::eSampled, vk::ImageLayout::eShaderReadOnlyOptimal, true);
        stbi_image_free(data);

        return std::make_tuple(image, format, vk::Extent2D(width, height));
    }

    std::shared_ptr<Image> GpuAllocator::init_image(uint32_t width, uint32_t height, uint32_t pixel_size, vk::Format format, unsigned char *data,
                                                    const vk::ImageUsageFlags &image_usage_flags, vk::ImageLayout il, bool gpu_only) const {
        auto buffer = init_buffer(data, width * height * pixel_size, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);

        auto image = create_image(vk::ImageCreateInfo({}, vk::ImageType::e2D, format, vk::Extent3D(width, height, 1), 1, 1, vk::SampleCountFlagBits::e1,
                                                      gpu_only ? vk::ImageTiling::eOptimal : vk::ImageTiling::eLinear, vk::ImageUsageFlagBits::eTransferDst | image_usage_flags,
                                                      vk::SharingMode::eExclusive, {}, vk::ImageLayout::eUndefined),
                                  VMA_MEMORY_USAGE_GPU_ONLY);


        buffer->map_and_write(data, width * height * pixel_size, 0);

        vk::BufferImageCopy copy{};
        copy.bufferOffset                    = 0;
        copy.bufferRowLength                 = 0;
        copy.bufferImageHeight               = 0;
        copy.imageSubresource.aspectMask     = vk::ImageAspectFlagBits::eColor;
        copy.imageSubresource.mipLevel       = 0;
        copy.imageSubresource.baseArrayLayer = 0;
        copy.imageSubresource.layerCount     = 1;
        copy.imageOffset                     = vk::Offset3D{0, 0, 0};
        copy.imageExtent                     = vk::Extent3D{width, height, 1};

        m_context->single_time_commands([&](const vk::CommandBuffer &cmd) {
            transitionImageLayout(cmd, image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTransfer,
                                  vk::PipelineStageFlagBits::eTransfer);
            cmd.copyBufferToImage(buffer->handle(), image->handle(), vk::ImageLayout::eTransferDstOptimal, copy);
            transitionImageLayout(cmd, image, vk::ImageLayout::eUndefined, il, vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer);
        });

        return image;
    }

    ImageView::ImageView(const std::shared_ptr<Context> &context, const Description &desc) : m_context(context), m_description(desc) {
        vk::ImageViewCreateInfo ivci{};
        ivci.image            = m_description.image->handle();
        ivci.viewType         = m_description.type;
        ivci.format           = m_description.format;
        ivci.components       = m_description.component_mapping;
        ivci.subresourceRange = m_description.subresource_range;

        vk::ImageViewUsageCreateInfo ivuci{};

        if (m_description.restricted_usage.has_value()) {
            ivuci.usage = m_description.restricted_usage.value();
            ivci.pNext  = &ivuci;
        }

        m_image_view = m_context->device().createImageView(ivci);
    }

    ImageView::~ImageView() {
        m_context->device().destroy(m_image_view);
    }

    Sampler::Sampler(const std::shared_ptr<Context> &context, const Description &desc) : m_context(context) {
        vk::SamplerCreateInfo sci{};
        sci.magFilter               = desc.mag_filter;
        sci.minFilter               = desc.min_filter;
        sci.mipmapMode              = desc.mipmap_mode;
        sci.addressModeU            = desc.address_mode_u;
        sci.addressModeV            = desc.address_mode_v;
        sci.addressModeW            = desc.address_mode_w;
        sci.mipLodBias              = desc.min_lod_bias;
        sci.anisotropyEnable        = desc.enable_anisotropy;
        sci.maxAnisotropy           = desc.max_anisotropy;
        sci.compareEnable           = desc.enable_compare;
        sci.compareOp               = desc.compare_op;
        sci.minLod                  = desc.min_lod;
        sci.maxLod                  = desc.max_lod;
        sci.borderColor             = desc.border_color;
        sci.unnormalizedCoordinates = desc.unnormalized_coordinates;

        m_sampler = m_context->device().createSampler(sci);
    }

    Sampler::~Sampler() {
        m_context->device().destroy(m_sampler);
    }

    void transitionImageLayout(const vk::CommandBuffer &cmd, const std::shared_ptr<Image> &image, vk::ImageLayout initial_layout, vk::ImageLayout final_layout,
                               vk::PipelineStageFlagBits source_stage, vk::PipelineStageFlagBits dest_stage) {
        vk::ImageMemoryBarrier imb{};
        imb.image               = image->handle();
        imb.oldLayout           = initial_layout;
        imb.newLayout           = final_layout;
        imb.srcAccessMask       = vk::AccessFlagBits::eNone;
        imb.dstAccessMask       = vk::AccessFlagBits::eNone;
        imb.subresourceRange    = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
        imb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        cmd.pipelineBarrier(source_stage, dest_stage, vk::DependencyFlags{}, {}, {}, imb);
    }
} // namespace kat
