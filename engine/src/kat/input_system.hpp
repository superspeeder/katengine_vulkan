#pragma once

#include <GLFW/glfw3.h>

#include <functional>
#include <eventpp/callbacklist.h>
#include <eventpp/eventdispatcher.h>
#include <map>

namespace kat {

    class InputSystem {
      public:
        using keyevent_signature = void(int scancode, bool pressed, int mods);
        using keyevent_dispatcher = eventpp::EventDispatcher<int, keyevent_signature>;
        using keyevent_handle = keyevent_dispatcher::Handle;

        explicit InputSystem(GLFWwindow* window);

        ~InputSystem() = default;

        keyevent_handle on_key_event(int key, const std::function<keyevent_signature>& f);

        void unregister_key_event(int key, const keyevent_handle& handle);

        void emit_key_event(int key, int scancode, bool pressed, int mods);

        static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

      private:


        keyevent_dispatcher m_key_event_dispatcher;

        GLFWwindow* m_window;
    };

} // namespace kat
