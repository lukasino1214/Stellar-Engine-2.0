#pragma once

#include <graphics/camera.hpp>
#include "panel/asset_browser_panel.hpp"
#include "panel/performance_stats_panel.hpp"
#include "panel/scene_hiearchy_panel.hpp"
#include "panel/toolbar_panel.hpp"
#include "panel/logger_panel.hpp"
#include "panel/viewport_panel.hpp"

#include <daxa/daxa.hpp>
#include <daxa/pipeline.hpp>
#include <daxa/utils/pipeline_manager.hpp>
#include <daxa/utils/imgui.hpp>

#include <core/window.hpp>
#include <graphics/model.hpp>

#include <systems/ssao_system.hpp>
#include <systems/deffered_rendering_system.hpp>

namespace Stellar {
    struct Context {
        daxa::Context context;
        daxa::Device device;
        daxa::PipelineManager pipeline_manager;
    };


    struct Editor {
        Editor(Context&& _context, std::shared_ptr<Stellar::Window>&& _window, daxa::Swapchain&& _swapchain, const std::string_view& project_path);
        ~Editor();

        void run();
        void ui_update();
        void on_update();
        void render();

        void on_resize(u32 sx, u32 sy);
        void on_key(i32 key, i32 action);
        void on_mouse_move(f32 x, f32 y);
        void on_mouse_button(i32 button, i32 action);

        Context context;
        std::shared_ptr<Stellar::Window> window;
        daxa::Swapchain swapchain;
        daxa::ImGuiRenderer imgui_renderer;

        u32 size_x;
        u32 size_y;

        daxa::ImageId displayed_image;

        bool show_render = false;
        bool show_albedo = false;
        bool show_normal = false;
        bool show_ssao = false;

        daxa::SamplerId sampler;

        std::shared_ptr<daxa::RasterPipeline> billboard_pipeline;
        std::shared_ptr<daxa::RasterPipeline> lines_pipeline;
        std::shared_ptr<daxa::RasterPipeline> atmosphere_pipeline;

        std::unique_ptr<SceneHiearchyPanel> scene_hiearchy_panel;
        std::unique_ptr<AssetBrowserPanel> asset_browser_panel;
        std::unique_ptr<ToolbarPanel> toolbar_panel;
        std::unique_ptr<PerformanceStatsPanel> performance_stats_panel;
        std::unique_ptr<LoggerPanel> logger_panel;
        std::unique_ptr<ViewportPanel> viewport_panel;

        std::unique_ptr<SSAOSystem> ssao_system;
        std::unique_ptr<DefferedRenderingSystem> deffered_rendering_system;

        std::unique_ptr<Texture> directional_light_texture;
        std::unique_ptr<Texture> point_light_texture;
        std::unique_ptr<Texture> spot_light_texture;

        u32 FPS = 0;
        f32 deltaTime = 0;
        f32 upTime = 0;
        f64 lastFrameTime = 0;

        ControlledCamera3D editor_camera;
        daxa::BufferId editor_camera_buffer;
        std::shared_ptr<Scene> scene;

        std::unique_ptr<Model> cube_model;
    };
}