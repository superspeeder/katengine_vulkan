#include "kat/graphics/window.hpp"
#include "kat/graphics/context.hpp"

namespace kat {
    Window::Window(const WindowSettings &settings) {

        glm::ivec2 size = settings.size;

        glfwInit();
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        if (settings.fullscreen) {
            auto monitor = glfwGetPrimaryMonitor();
            auto vm      = glfwGetVideoMode(monitor);

            glm::ivec2 mp;
            glfwGetMonitorPos(monitor, &mp.x, &mp.y);
            glfwWindowHint(GLFW_POSITION_X, mp.x);
            glfwWindowHint(GLFW_POSITION_Y, mp.y);
            glfwWindowHint(GLFW_DECORATED, false);

            size = {vm->width, vm->height};
        } else {
            glfwWindowHint(GLFW_POSITION_X, settings.pos.x);
            glfwWindowHint(GLFW_POSITION_Y, settings.pos.y);
        }

        glfwWindowHint(GLFW_RESIZABLE, false);

        m_window = glfwCreateWindow(size.x, size.y, settings.title.c_str(), nullptr, nullptr);
        
        glfwGetFramebufferSize(m_window, &m_size.x, &m_size.y);
    } // namespace kat

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

    bool Window::get_key(int key) const {
        return glfwGetKey(m_window, key) == GLFW_PRESS;
    }
} // namespace kat
