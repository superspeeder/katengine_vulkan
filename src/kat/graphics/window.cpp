#include "kat/graphics/window.hpp"
#include "kat/graphics/context.hpp"

namespace kat {
    Window::Window(const WindowSettings &settings) {
        glfwInit();
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_POSITION_X, settings.pos.x);
        glfwWindowHint(GLFW_POSITION_Y, settings.pos.y);
        glfwWindowHint(GLFW_RESIZABLE, false);

        m_window = glfwCreateWindow(settings.size.x, settings.size.y, settings.title.c_str(), nullptr, nullptr);
    }

    Window::~Window() {
        glfwDestroyWindow(m_window);
    }

    bool Window::is_open() const {
        return !glfwWindowShouldClose(m_window);
    }

    void Window::poll() {
        glfwPollEvents();
    }

    vk::SurfaceKHR Window::create_surface(const vk::Instance &instance) const {
        VkSurfaceKHR s;
        glfwCreateWindowSurface(instance, m_window, nullptr, &s);
        return s;
    }
} // namespace kat
