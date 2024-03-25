#pragma once

#include "kat/graphics/context.hpp"
#include "kat/graphics/graphics_pipeline.hpp"
#include "kat/graphics/window.hpp"

#include <imgui.h>

#include <atomic>
#include <thread>

namespace kat {

    struct ImGuiResources {
        explicit ImGuiResources(const std::unique_ptr<Window> &window, const std::shared_ptr<Context> &context);

        std::shared_ptr<DescriptorPool> descriptor_pool;
    };

    class App {
      public:
        App(const WindowSettings &window_settings, const ContextSettings &context_settings);
        virtual ~App() = default;

        virtual void update(float dt)                                   = 0;
        virtual void render(const kat::FrameInfo &frame_info, float dt) = 0;

        void launch();

        [[nodiscard]] inline const std::unique_ptr<Window> &window() const { return m_window; }

        [[nodiscard]] inline const std::shared_ptr<Context> &context() const { return m_context; }

        [[nodiscard]] inline float this_frame() const { return m_this_frame; }

        [[nodiscard]] inline float last_frame() const { return m_last_frame; }

        [[nodiscard]] inline float render_delta() const { return m_render_delta; }

        [[nodiscard]] inline float this_update() const { return m_this_update; }

        [[nodiscard]] inline float last_update() const { return m_last_update; }

        [[nodiscard]] inline float update_delta() const { return m_update_delta; }

      protected:
        std::unique_ptr<Window>  m_window;
        std::shared_ptr<Context> m_context;


      private:
        std::jthread m_render_thread;
        bool         m_is_running = true; // actually doesn't need to be atomic since it's only going to be changed by 1 thread, read by 1 (different) thread.

        float m_this_frame, m_last_frame, m_render_delta;
        float m_this_update, m_last_update, m_update_delta;
    };

} // namespace kat
