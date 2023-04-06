#include "editor.hpp"
#include "core/types.hpp"
#include "data/components.hpp"
#include "panel/asset_browser_panel.hpp"
#include "panel/performance_stats_panel.hpp"
#include "panel/scene_hiearchy_panel.hpp"
#include "panel/toolbar_panel.hpp"

#include <memory>
#include <thread>

#include <imgui.h>
#include <imgui_impl_glfw.h>

#include <data/scene.hpp>
#include <data/entity.hpp>
#include <core/logger.hpp>

namespace Stellar {
    Editor::Editor(Context&& _context, Window&& _window, daxa::Swapchain&& _swapchain, const std::string_view& project_path) : context{_context}, window{_window}, swapchain{_swapchain} {
        window.toggle_border(true);
        window.set_size(1200, 720);
        window.set_position(1920 + 1920 / 2 - window.width / 2, 1080 / 2 - window.height / 2);
        window.set_name(std::string{"Stellar Editor - "} + std::string{project_path});
        swapchain.resize();

        using namespace std::literals;
        std::this_thread::sleep_for(50ms);

        swapchain.resize();
        swapchain.resize();

        ImGui_ImplGlfw_InitForVulkan(window.glfw_window_ptr, true);
        imgui_renderer =  daxa::ImGuiRenderer({
            .device = context.device,
            .format = swapchain.get_format(),
        });

        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        
        std::shared_ptr<Scene> scene = std::make_shared<Scene>("Test");
        auto e = scene->create_entity("pog");
        auto& tc = e.add_component<TransformComponent>();
        tc.position = { 1.0f, 2.0f, 3.0f };
        tc.rotation = { 80.0f, 60.0f, 20.0f };

        auto e1 = scene->create_entity("test");
        auto e2 = scene->create_entity("lol");

        e1.get_component<RelationshipComponent>().parent = e;
        e2.get_component<RelationshipComponent>().parent = e;
        e.get_component<RelationshipComponent>().children = { e1, e2 };

        scene->serialize("test.scene");
        scene->deserialize("test.scene");

        scene_hiearchy_panel = std::make_unique<SceneHiearchyPanel>(scene);
        asset_browser_panel = std::make_unique<AssetBrowserPanel>(project_path);
        toolbar_panel = std::make_unique<ToolbarPanel>();
        performance_stats_panel = std::make_unique<PerformanceStatsPanel>();
        logger_panel = std::make_unique<ImGuiConsole>();

        glfwSetWindowUserPointer(window.glfw_window_ptr, this);
        glfwSetFramebufferSizeCallback(window.glfw_window_ptr, [](GLFWwindow * window_ptr, i32 w, i32 h) {
            auto& app = *reinterpret_cast<Editor *>(glfwGetWindowUserPointer(window_ptr));
            app.on_resize(static_cast<u32>(w), static_cast<u32>(h));
        });

        size_x = swapchain.get_surface_extent().x;
        size_y = swapchain.get_surface_extent().y;

        Logger::init();
        CORE_INFO("Test");
    }

    Editor::~Editor() {
        ImGui_ImplGlfw_Shutdown();

        context.device.wait_idle();
        context.device.collect_garbage();
    }

    void Editor::run() {
        while(!window.should_close()) {
            glfwPollEvents();

            ui_update();
            render();
        }
    }

    void Editor::ui_update() {
        f64 currentFrameTime = glfwGetTime();

        deltaTime = static_cast<f32>(currentFrameTime - lastFrameTime);
        lastFrameTime = currentFrameTime;

        upTime = static_cast<f32>(currentFrameTime);
        FPS = static_cast<u32>(glm::ceil(1.0f / deltaTime));

        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        {
            ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
            ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBackground;
            const ImGuiViewport *viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::Begin("DockSpace Demo", nullptr, window_flags);
            ImGui::PopStyleVar(3);
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
            ImGui::End();
        }

        performance_stats_panel->fps = FPS;
        performance_stats_panel->delta_time = deltaTime;
        performance_stats_panel->up_time = upTime;

        scene_hiearchy_panel->draw();
        asset_browser_panel->draw();
        toolbar_panel->draw();
        performance_stats_panel->draw();
        bool open = true;
        logger_panel->draw("Logger Panel", &open);

        ImGui::Begin("Viewport");
        ImGui::End();

        ImGui::Render();
    }

    void Editor::render() {
        daxa::ImageId swapchain_image = swapchain.acquire_next_image();
        if(swapchain_image.is_empty()) { return; }

        daxa::CommandList cmd_list = context.device.create_command_list({ .debug_name = "main loop cmd list" });

        cmd_list.pipeline_barrier_image_transition({
            .waiting_pipeline_access = daxa::AccessConsts::TRANSFER_WRITE,
            .before_layout = daxa::ImageLayout::UNDEFINED,
            .after_layout = daxa::ImageLayout::TRANSFER_DST_OPTIMAL,
            .image_id = swapchain_image
        });

        cmd_list.clear_image({
            .dst_image_layout = daxa::ImageLayout::TRANSFER_DST_OPTIMAL,
            .clear_value = std::array<daxa::f32, 4>{0.00368f, 0.00368f, 0.00368f, 1.0},
            .dst_image = swapchain_image
        });

        imgui_renderer.record_commands(ImGui::GetDrawData(), cmd_list, swapchain_image, size_x, size_y);

        cmd_list.pipeline_barrier_image_transition({
            .awaited_pipeline_access = daxa::AccessConsts::ALL_GRAPHICS_READ_WRITE,
            .before_layout = daxa::ImageLayout::TRANSFER_DST_OPTIMAL,
            .after_layout = daxa::ImageLayout::PRESENT_SRC,
            .image_id = swapchain_image
        });

        cmd_list.complete();

        context.device.submit_commands({
            .command_lists = {std::move(cmd_list)},
            .wait_binary_semaphores = {swapchain.get_acquire_semaphore()},
            .signal_binary_semaphores = {swapchain.get_present_semaphore()},
            .signal_timeline_semaphores = {{swapchain.get_gpu_timeline_semaphore(), swapchain.get_cpu_timeline_value()}},
        });

        context.device.present_frame({
            .wait_binary_semaphores = {swapchain.get_present_semaphore()},
            .swapchain = swapchain,
        });
    }

    void Editor::on_resize(u32 sx, u32 sy) {
        //window.minimized = (sx == 0 || sy == 0);
        //if (!window.minimized) {
            //rendering_system->resize(sx, sy);
            swapchain.resize();
            size_x = swapchain.get_surface_extent().x;
            size_y = swapchain.get_surface_extent().y;
            window.width = static_cast<i32>(size_x);
            window.height = static_cast<i32>(size_y);
            render();
        //}
    }
}