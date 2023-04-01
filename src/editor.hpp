#pragma once

#include <daxa/daxa.hpp>
#include <daxa/utils/pipeline_manager.hpp>
#include <daxa/utils/imgui.hpp>

#include "window.hpp"

namespace Stellar {
    struct Context {
        daxa::Context context;
        daxa::Device device;
        daxa::PipelineManager pipeline_manager;
    };


    struct Editor {
        Editor(Context&& _context, Window&& _window, daxa::Swapchain&& _swapchain, const std::string_view& project_path);
        ~Editor();

        void run();
        void ui_update();
        void on_update();
        void render();

        Context context;
        Window window;
        daxa::Swapchain swapchain;
        daxa::ImGuiRenderer imgui_renderer;
    };
}