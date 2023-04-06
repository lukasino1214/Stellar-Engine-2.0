#pragma once

#include "panel/asset_browser_panel.hpp"
#include "panel/performance_stats_panel.hpp"
#include "panel/scene_hiearchy_panel.hpp"
#include "panel/toolbar_panel.hpp"
#include "panel/logger_panel.hpp"

#include <daxa/daxa.hpp>
#include <daxa/utils/pipeline_manager.hpp>
#include <daxa/utils/imgui.hpp>

#include <core/window.hpp>

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

        void on_resize(u32 sx, u32 sy);

        Context context;
        Window window;
        daxa::Swapchain swapchain;
        daxa::ImGuiRenderer imgui_renderer;

        u32 size_x;
        u32 size_y;

        std::unique_ptr<SceneHiearchyPanel> scene_hiearchy_panel;
        std::unique_ptr<AssetBrowserPanel> asset_browser_panel;
        std::unique_ptr<ToolbarPanel> toolbar_panel;
        std::unique_ptr<PerformanceStatsPanel> performance_stats_panel;
        std::unique_ptr<ImGuiConsole> logger_panel;

        u32 FPS = 0;
        f32 deltaTime = 0;
        f32 upTime = 0;
        f64 lastFrameTime = 0;
    };
}