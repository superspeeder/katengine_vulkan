#pragma once

#include <VkBootstrap.h>
#include <memory>
#include <vulkan/vulkan.hpp>

#include "kat/graphics/window.hpp"

#include "kat/util/util.hpp"

#include <vk_mem_alloc.h>

namespace kat {

    enum class QueueType { GRAPHICS, PRESENT, TRANSFER, COMPUTE };

    struct Version {
        uint32_t major, minor, patch;
    };

    struct ContextSettings {
        std::string app_name    = "App";
        Version     app_version = {0, 1, 0};
    };

    struct FrameInfo {
        vk::Semaphore image_available_semaphore;
        vk::Semaphore render_finished_semaphore;
        vk::Image     image;
        vk::ImageView image_view;
        vk::Fence     in_flight_fence;

        // this should be used to get resources created from the image set like framebuffers (which are out of the scope of the context to manage).
        uint32_t image_index;
    };

    constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    class ShaderCache;

    class GpuAllocator;

    class Context : public std::enable_shared_from_this<Context> {
        explicit Context(const std::unique_ptr<Window> &window, const ContextSettings &settings = {});

        void init();

      public:
        static inline std::shared_ptr<Context> init(const std::unique_ptr<Window> &window, const ContextSettings &settings = {}) {
            auto ctx = std::shared_ptr<Context>(new Context(window, settings));
            ctx->init();
            return ctx;
        };

        ~Context();

        [[nodiscard]] inline const vkb::Instance &vkb_instance() const { return m_inst; };

        [[nodiscard]] inline const vkb::PhysicalDevice &vkb_physical_device() const { return m_phys; };

        [[nodiscard]] inline const vkb::Device &vkb_device() const { return m_dev; };

        [[nodiscard]] inline vk::Instance instance() const { return m_instance; };

        [[nodiscard]] inline vk::PhysicalDevice physical_device() const { return m_physical_device; };

        [[nodiscard]] inline vk::SurfaceKHR surface() const { return m_surface; };

        [[nodiscard]] inline vk::Device device() const { return m_device; };

        [[nodiscard]] inline vk::Queue graphics_queue() const { return m_graphics_queue; };

        [[nodiscard]] inline vk::Queue present_queue() const { return m_present_queue; };

        [[nodiscard]] inline vk::Queue transfer_queue() const { return m_transfer_queue; };

        [[nodiscard]] inline vk::Queue compute_queue() const { return m_compute_queue; };

        [[nodiscard]] inline uint32_t graphics_family() const { return m_graphics_family; };

        [[nodiscard]] inline uint32_t present_family() const { return m_present_family; };

        [[nodiscard]] inline uint32_t transfer_family() const { return m_transfer_family; };

        [[nodiscard]] inline uint32_t compute_family() const { return m_compute_family; };

        [[nodiscard]] inline const vkb::Swapchain &vkb_swapchain() const { return m_swc; };

        [[nodiscard]] inline vk::SwapchainKHR swapchain() const { return m_swapchain; };

        [[nodiscard]] inline uint32_t current_frame() const noexcept { return m_current_frame; };

        [[nodiscard]] inline const std::vector<vk::Image> &swapchain_images() const { return m_swc_images; };

        [[nodiscard]] inline const std::vector<vk::ImageView> &swapchain_image_views() const { return m_swc_image_views; };

        [[nodiscard]] inline vk::Extent2D swapchain_extent() const { return m_swc.extent; };

        [[nodiscard]] inline vk::Format swapchain_format() const { return static_cast<vk::Format>(m_swc.image_format); };

        [[nodiscard]] inline vk::Rect2D full_render_area() const { return vk::Rect2D{{0, 0}, m_swc.extent}; };

        [[nodiscard]] inline const std::unique_ptr<ShaderCache> &shader_cache() const { return m_shader_cache; };

        [[nodiscard]] inline vk::PipelineCache pipeline_cache() const { return VK_NULL_HANDLE; }; // TODO: pipeline caches.

        [[nodiscard]] inline vk::Viewport full_viewport() const {
            return vk::Viewport(0.0f, 0.0f, static_cast<float>(m_swc.extent.width), static_cast<float>(m_swc.extent.height), 0.0f, 1.0f);
        };

        [[nodiscard]] const std::unique_ptr<GpuAllocator> &gpu_allocator() const { return m_gpu_allocator; }

        void create_swapchain();

        [[nodiscard]] FrameInfo acquire_next_frame();

        vk::Semaphore create_semaphore() const;
        vk::Fence     create_fence(bool signaled) const;
        vk::Fence     create_fence() const;
        vk::Fence     create_fence_signaled() const;

        bool wait_for_fences(const std::vector<vk::Fence> &fences) const;

        void reset_fences(const std::vector<vk::Fence> &fences) const;

        template <uint32_t N>
        std::array<vk::Semaphore, N> create_semaphores() {
            std::array<vk::Semaphore, N> arr;
            for (uint32_t i = 0; i < N; i++)
                arr[i] = create_semaphore();
            return arr;
        };

        template <uint32_t N>
        std::array<vk::Fence, N> create_fences(bool signaled) {
            if (signaled)
                return create_fences_signaled<N>();
            return create_fences<N>();
        };

        template <uint32_t N>
        std::array<vk::Fence, N> create_fences() {
            std::array<vk::Fence, N> arr;
            for (uint32_t i = 0; i < N; i++)
                arr[i] = create_fence();
            return arr;
        };

        template <uint32_t N>
        std::array<vk::Fence, N> create_fences_signaled() {
            std::array<vk::Fence, N> arr;
            for (uint32_t i = 0; i < N; i++)
                arr[i] = create_fence_signaled();
            return arr;
        };

        vk::Queue get_queue(const QueueType &queue_type) const;
        uint32_t  get_queue_family(const QueueType &queue_type) const;

        template <QueueType Q>
        vk::Queue get_queue() const;

        template <QueueType Q>
        uint32_t get_queue_family() const;

        [[nodiscard]] vk::CommandPool create_command_pool_raw(uint32_t family, bool allow_individual_reset = true) const;

        template <QueueType Q>
        [[nodiscard]] vk::CommandPool create_command_pool_raw(bool allow_individual_reset = true) const {
            return create_command_pool_raw(get_queue_family<Q>(), allow_individual_reset);
        };

        template <uint32_t N>
        std::array<vk::CommandBuffer, N> allocate_command_buffers_raw(const vk::CommandPool &pool) const {
            std::vector<vk::CommandBuffer>   command_buffers = m_device.allocateCommandBuffers(vk::CommandBufferAllocateInfo(pool, vk::CommandBufferLevel::ePrimary, N));
            std::array<vk::CommandBuffer, N> arr;
            std::copy_n(command_buffers.begin(), N, arr.begin());

            return arr;
        };

        void present();

        [[nodiscard]] vk::CommandBuffer begin_single_time_commands() const;

        void end_single_time_commands(const vk::CommandBuffer &cmd) const;

      private:
        vkb::Instance       m_inst;
        vkb::PhysicalDevice m_phys;
        vkb::Device         m_dev;

        vk::Instance       m_instance;
        vk::PhysicalDevice m_physical_device;
        vk::Device         m_device;
        vk::SurfaceKHR     m_surface;

        vk::Queue m_graphics_queue;
        vk::Queue m_present_queue;
        vk::Queue m_transfer_queue;
        vk::Queue m_compute_queue;

        uint32_t m_graphics_family;
        uint32_t m_present_family;
        uint32_t m_transfer_family;
        uint32_t m_compute_family;

        vkb::Swapchain   m_swc;
        vk::SwapchainKHR m_swapchain;

        std::vector<vk::Image>     m_swc_images;
        std::vector<vk::ImageView> m_swc_image_views;

        uint32_t  m_current_frame = 0;
        FrameInfo m_current_frame_info;

        uint32_t m_current_image_index;

        std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> m_image_available_semaphores;
        std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> m_render_finished_semaphores;
        std::array<vk::Fence, MAX_FRAMES_IN_FLIGHT>     m_in_flight_fences;

        std::unique_ptr<ShaderCache>  m_shader_cache;
        std::unique_ptr<GpuAllocator> m_gpu_allocator;

        vk::CommandPool m_single_time_gt_pool;
    };

    class Buffer {
      public:
        Buffer(const std::shared_ptr<Context> &context, vk::Buffer buffer, VmaAllocation allocation, VmaAllocationInfo allocation_info);

        ~Buffer();

        void map_and_write(const void *data, const vk::DeviceSize &size, const vk::DeviceSize &offset) const;

        template <typename T>
        void map_and_write(const std::vector<T> &data, const vk::DeviceSize &offset) const {
            map_and_write(data.data(), data.size() * sizeof(T), offset);
        };

        void copy_from(const std::shared_ptr<Buffer> &other, const vk::DeviceSize &size, const vk::DeviceSize &src_offset, const vk::DeviceSize &dst_offset) const;

        [[nodiscard]] inline vk::Buffer handle() const { return m_buffer; };

      private:
        vk::Buffer        m_buffer;
        VmaAllocation     m_allocation;
        VmaAllocationInfo m_allocation_info;

        std::shared_ptr<Context> m_context;
    };

    class Image {
      public:
        Image(const std::shared_ptr<Context> &context, vk::Image image, VmaAllocation allocation, VmaAllocationInfo allocation_info);

        ~Image();

        [[nodiscard]] inline vk::Image handle() const { return m_image; };

      private:
        vk::Image         m_image;
        VmaAllocation     m_allocation;
        VmaAllocationInfo m_allocation_info;

        std::shared_ptr<Context> m_context;
    };

    class GpuAllocator : public std::enable_shared_from_this<GpuAllocator> {
      public:
        explicit GpuAllocator(const std::shared_ptr<Context> &context);

        ~GpuAllocator();

        [[nodiscard]] std::shared_ptr<Buffer> create_buffer(const vk::BufferCreateInfo &create_info, const VmaMemoryUsage &vma_memory_usage = VMA_MEMORY_USAGE_AUTO) const;
        [[nodiscard]] std::shared_ptr<Image>  create_image(const vk::ImageCreateInfo &create_info, const VmaMemoryUsage &vma_memory_usage = VMA_MEMORY_USAGE_AUTO) const;

        [[nodiscard]] inline std::shared_ptr<Buffer> create_buffer(const vk::DeviceSize &size, const vk::BufferUsageFlags usage,
                                                                   const VmaMemoryUsage &vma_memory_usage = VMA_MEMORY_USAGE_AUTO) const {
            return create_buffer(vk::BufferCreateInfo({}, size, usage, vk::SharingMode::eExclusive, {}), vma_memory_usage);
        };

        void free_buffer(const vk::Buffer &buffer, const VmaAllocation &allocation) const;
        void free_image(const vk::Image &imageimage, const VmaAllocation &allocation) const;

        template <typename T>
        [[nodiscard]] std::shared_ptr<Buffer> init_buffer(const std::vector<T> &data, const vk::BufferUsageFlags usage,
                                                          const VmaMemoryUsage &vma_memory_usage = VMA_MEMORY_USAGE_AUTO) const {
            return init_buffer(data.data(), data.size() * sizeof(T), usage, vma_memory_usage);
        };

        [[nodiscard]] std::shared_ptr<Buffer> init_buffer(const void *data, const vk::DeviceSize &size, const vk::BufferUsageFlags usage,
                                                          const VmaMemoryUsage &vma_memory_usage = VMA_MEMORY_USAGE_AUTO) const;

        [[nodiscard]] void *map(const VmaAllocation &alloc) const;

        void unmap(const VmaAllocation &alloc) const;

      private:
        std::shared_ptr<Context> m_context;
        VmaAllocator             m_allocator = VK_NULL_HANDLE;
    };

    template <>
    inline vk::Queue Context::get_queue<QueueType::GRAPHICS>() const {
        return m_graphics_queue;
    };

    template <>
    inline vk::Queue Context::get_queue<QueueType::PRESENT>() const {
        return m_present_queue;
    };

    template <>
    inline vk::Queue Context::get_queue<QueueType::TRANSFER>() const {
        return m_transfer_queue;
    };

    template <>
    inline vk::Queue Context::get_queue<QueueType::COMPUTE>() const {
        return m_compute_queue;
    };

    template <>
    inline uint32_t Context::get_queue_family<QueueType::GRAPHICS>() const {
        return m_graphics_family;
    };

    template <>
    inline uint32_t Context::get_queue_family<QueueType::PRESENT>() const {
        return m_present_family;
    };

    template <>
    inline uint32_t Context::get_queue_family<QueueType::TRANSFER>() const {
        return m_transfer_family;
    };

    template <>
    inline uint32_t Context::get_queue_family<QueueType::COMPUTE>() const {
        return m_compute_family;
    };
} // namespace kat
