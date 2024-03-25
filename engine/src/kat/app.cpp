#include "app.hpp"

namespace kat {
    App::App(const WindowSettings &window_settings, const ContextSettings &context_settings) {
        glfwInit();
        m_window = std::make_unique<Window>(window_settings);

        m_context = kat::Context::init(m_window, context_settings);

        m_this_frame   = static_cast<float>(glfwGetTime());
        m_this_update  = static_cast<float>(glfwGetTime());
        m_render_delta = 1.0 / 60.0;
        m_update_delta = 1.0 / 60.0;
        m_last_frame   = m_this_frame - m_render_delta;
        m_last_update  = m_this_frame - m_render_delta;
    }

    void App::launch() {
        m_render_thread = std::jthread([this]() {
            while (m_is_running) {
                auto frame_info = m_context->acquire_next_frame();

                render(frame_info, m_render_delta);

                m_context->present();

                m_last_frame = m_this_frame;
                m_last_frame = static_cast<float>(glfwGetTime());
                m_render_delta = m_this_frame - m_last_frame;
            }
        });

        while (m_window->is_open()) {
            kat::Window::poll();

            update(m_update_delta);

            m_last_update = m_this_update;
            m_this_update = static_cast<float>(glfwGetTime());
            m_update_delta = m_this_update - m_last_update;
        }

        m_is_running = false;
        m_render_thread.join();

        m_context->device().waitIdle();
    }
} // namespace kat
