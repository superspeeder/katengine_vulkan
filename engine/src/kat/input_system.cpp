#include "input_system.hpp"

#include "kat/graphics/window.hpp"

namespace kat {
    InputSystem::InputSystem(GLFWwindow *window) : m_window(window) {}

    InputSystem::keyevent_handle InputSystem::on_key_event(int key, const std::function<keyevent_signature> &f) {
        return m_key_event_dispatcher.appendListener(key, f);
    }

    void InputSystem::unregister_key_event(int key, const keyevent_dispatcher::Handle& handle) {
        m_key_event_dispatcher.removeListener(key, handle);
    }

    void InputSystem::emit_key_event(int key, int scancode, bool pressed, int mods) {

    }

    void InputSystem::key_callback(GLFWwindow *window_, int key, int scancode, int action, int mods) {

        /// event capture resolvers: input system tracks a list of functions per input type which determine if input should be passed to application events or not. These will be populated by the engine upon connection to the window.

        Window* window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window_));
        if (window) {
            window->input_system()->emit_key_event(key, scancode, action == GLFW_PRESS, mods);
        }

    }
} // namespace kat
