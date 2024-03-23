#pragma once

#include <string>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <vulkan/vulkan.hpp>

namespace kat {
    class Context;

    struct WindowSettings {
        glm::ivec2  size{800, 600};
        glm::ivec2  pos{GLFW_ANY_POSITION, GLFW_ANY_POSITION};
        std::string title      = "Window";
        bool        fullscreen = false;
    };

    class Window {
      public:
        explicit Window(const WindowSettings &settings = {});

        ~Window();

        [[nodiscard]] bool is_open() const;

        static void poll();

        [[nodiscard]] inline GLFWwindow *window() const { return m_window; }

        [[nodiscard]] bool get_key(int key) const;

        [[nodiscard]] vk::SurfaceKHR create_surface(const vk::Instance &instance) const;

        [[nodiscard]] const glm::ivec2 &size() const { return m_size; };

      private:
        GLFWwindow *m_window;
        glm::ivec2  m_size;
    };
} // namespace kat
