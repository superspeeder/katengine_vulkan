#pragma once
#include <memory>

#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

#include "kat/app.hpp"
#include "kat/graphics/context.hpp"
#include "kat/graphics/graphics_pipeline.hpp"
#include "kat/graphics/render_pass.hpp"
#include "kat/graphics/shader_cache.hpp"
#include "kat/graphics/window.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <atomic>
#include <thread>

namespace game {

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

        float ambient_strength;
        float specular_strength;
    };

    class Game : public kat::App {
      public:
        Game();
        ~Game() override = default;

        void create_render_pass();
        void create_pipeline_layout();
        void create_graphics_pipeline();
        void create_buffers();

        void update(float dt) override;
        void render(const kat::FrameInfo &frame_info, float dt) override;

        void update_ubo();

      private:
        glm::vec3  m_pos = {0.0f, 0.0f, -2.0f};
        glm::fquat m_rot = glm::identity<glm::fquat>();

        std::shared_ptr<kat::RenderPass>          m_render_pass;
        std::shared_ptr<kat::DescriptorSetLayout> m_descriptor_set_layout;
        std::shared_ptr<kat::PipelineLayout>      m_pipeline_layout;
        std::shared_ptr<kat::GraphicsPipeline>    m_graphics_pipeline;
        std::shared_ptr<kat::DescriptorPool>      m_descriptor_pool;
        std::vector<vk::Framebuffer>              m_framebuffers;

        vk::CommandPool                                          m_command_pool;
        std::array<vk::CommandBuffer, kat::MAX_FRAMES_IN_FLIGHT> m_command_buffers;

        std::shared_ptr<kat::Buffer> m_index_buffer;
        std::shared_ptr<kat::Buffer> m_vertex_buffer;

        std::vector<std::shared_ptr<kat::Buffer>> m_uniform_buffers;
        std::vector<vk::DescriptorSet>            m_descriptor_sets;
    };

} // namespace game
