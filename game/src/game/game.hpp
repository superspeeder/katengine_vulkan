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
        glm::vec2 tex_coords;
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
        Game(const std::filesystem::path& resources_dir);
        ~Game() override = default;

        void create_render_pass();
        void create_pipeline_layout();
        void create_graphics_pipeline();
        void create_buffers();

        void render_ui();

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

        std::unique_ptr<kat::ImGuiResources> m_imgui_resources;

        bool m_demowindow_open = true;

        kat::color m_background_color = kat::BLACK;
        kat::color m_light_color = kat::WHITE;
        kat::color m_ambient_light_color = kat::WHITE;
        float m_ambient_strength = 0.05f;
        float m_specular_strength = 0.5f;
        glm::vec4 m_light_pos = glm::vec4{0.5f, -2.0f, 1.5f, 1.0f};

        std::shared_ptr<kat::Image> m_test_image;
        std::shared_ptr<kat::ImageView> m_test_image_view;
        std::shared_ptr<kat::Sampler> m_test_sampler;

        bool m_ui_toggled = false;
    };

} // namespace game
