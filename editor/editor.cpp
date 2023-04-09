#include "editor.hpp"
#include <core/types.hpp>
#include <data/components.hpp>

#include <daxa/types.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/trigonometric.hpp>
#include <thread>

#include <imgui.h>
#include <imgui_impl_glfw.h>

#include <data/scene.hpp>
#include <data/entity.hpp>
#include <core/logger.hpp>

#include "../shaders/shared.inl"

namespace Stellar {
    Editor::Editor(Context&& _context, std::shared_ptr<Stellar::Window>&& _window, daxa::Swapchain&& _swapchain, const std::string_view& project_path) : context{_context}, window{_window}, swapchain{_swapchain} {
        window->toggle_border(true);
        window->set_size(1200, 720);
        window->set_position(1920 + 1920 / 2 - window->width / 2, 1080 / 2 - window->height / 2);
        window->set_name(std::string{"Stellar Editor - "} + std::string{project_path});
        swapchain.resize();

        using namespace std::literals;
        std::this_thread::sleep_for(50ms);

        swapchain.resize();
        swapchain.resize();
        
        scene = std::make_shared<Scene>("Test", context.device);
        scene->deserialize("test.scene");

        scene_hiearchy_panel = std::make_unique<SceneHiearchyPanel>(scene);
        asset_browser_panel = std::make_unique<AssetBrowserPanel>(project_path);
        toolbar_panel = std::make_unique<ToolbarPanel>(window);
        performance_stats_panel = std::make_unique<PerformanceStatsPanel>();
        logger_panel = std::make_unique<ImGuiConsole>();

        glfwSetWindowUserPointer(window->glfw_window_ptr, this);
        glfwSetFramebufferSizeCallback(window->glfw_window_ptr, [](GLFWwindow * window_ptr, i32 w, i32 h) {
            auto& app = *reinterpret_cast<Editor *>(glfwGetWindowUserPointer(window_ptr));
            app.on_resize(static_cast<u32>(w), static_cast<u32>(h));
        });
        glfwSetCursorPosCallback(window->glfw_window_ptr, [](GLFWwindow * window_ptr, f64 x, f64 y) {
            auto & app = *reinterpret_cast<Editor *>(glfwGetWindowUserPointer(window_ptr));
            app.on_mouse_move(static_cast<f32>(x), static_cast<f32>(y));
        });
        glfwSetKeyCallback(window->glfw_window_ptr, [](GLFWwindow * window_ptr, i32 key, i32, i32 action, i32) {
            auto & app = *reinterpret_cast<Editor *>(glfwGetWindowUserPointer(window_ptr));
            app.on_key(key, action);
        });

        ImGui_ImplGlfw_InitForVulkan(window->glfw_window_ptr, true);
        imgui_renderer =  daxa::ImGuiRenderer({
            .device = context.device,
            .format = swapchain.get_format(),
        });
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        size_x = swapchain.get_surface_extent().x;
        size_y = swapchain.get_surface_extent().y;

        viewport_size_x = swapchain.get_surface_extent().x;
        viewport_size_y = swapchain.get_surface_extent().y;

        Logger::init();
        CORE_INFO("Test");

        render_image = context.device.create_image({
            .format = daxa::Format::R8G8B8A8_UNORM,
            .aspect = daxa::ImageAspectFlagBits::COLOR,
            .size = { size_x, size_y, 1 },
            .usage = daxa::ImageUsageFlagBits::COLOR_ATTACHMENT | daxa::ImageUsageFlagBits::SHADER_READ_ONLY | daxa::ImageUsageFlagBits::TRANSFER_DST,
            .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
            .debug_name = "render_image"
        });

        depth_image = context.device.create_image({
            .format = daxa::Format::D32_SFLOAT,
            .aspect = daxa::ImageAspectFlagBits::DEPTH,
            .size = { size_x, size_y, 1 },
            .usage = daxa::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT,
            .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
            .debug_name = "depth_image"
        });

        depth_prepass_pipeline = context.pipeline_manager.add_raster_pipeline({
            .vertex_shader_info = {.source = daxa::ShaderFile{"depth_prepass.glsl"}, .compile_options = {.defines = {daxa::ShaderDefine{"DRAW_VERT"}}}},
            .fragment_shader_info = {.source = daxa::ShaderFile{"depth_prepass.glsl"}, .compile_options = {.defines = {daxa::ShaderDefine{"DRAW_FRAG"}}}},
            .depth_test = {
                .depth_attachment_format = daxa::Format::D32_SFLOAT,
                .enable_depth_test = true,
                .enable_depth_write = true,
            },
            .raster = {
                .face_culling = daxa::FaceCullFlagBits::FRONT_BIT
            },
            .push_constant_size = sizeof(DepthPrepassPush),
            .debug_name = "raster_pipeline",
        }).value();

        raster_pipeline = context.pipeline_manager.add_raster_pipeline({
            .vertex_shader_info = {.source = daxa::ShaderFile{"draw.glsl"}, .compile_options = {.defines = {daxa::ShaderDefine{"DRAW_VERT"}}}},
            .fragment_shader_info = {.source = daxa::ShaderFile{"draw.glsl"}, .compile_options = {.defines = {daxa::ShaderDefine{"DRAW_FRAG"}}}},
            .color_attachments = {{ .format = daxa::Format::R8G8B8A8_UNORM }},
            .depth_test = {
                .depth_attachment_format = daxa::Format::D32_SFLOAT,
                .enable_depth_test = true,
                .enable_depth_write = false,
            },
            .raster = {
                .face_culling = daxa::FaceCullFlagBits::FRONT_BIT
            },
            .push_constant_size = sizeof(DrawPush),
            .debug_name = "raster_pipeline",
        }).value();

        editor_camera.camera.resize(size_x, size_y);

        Entity e = scene->create_entity("Directional Light");
        e.add_component<TransformComponent>();
        e.add_component<DirectionalLightComponent>();

        editor_camera_buffer = context.device.create_buffer({
            .memory_flags = daxa::MemoryFlagBits::HOST_ACCESS_RANDOM,
            .size = sizeof(CameraInfo),
            .debug_name = "editor camera buffer"
        });
    }

    Editor::~Editor() {
        ImGui_ImplGlfw_Shutdown();

        context.device.wait_idle();
        context.device.collect_garbage();
        context.device.destroy_image(render_image);
        context.device.destroy_image(depth_image);
        context.device.destroy_buffer(editor_camera_buffer);
    }

    void Editor::run() {
        while(!window->should_close()) {
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

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Viewport");
        ImVec2 v = ImGui::GetContentRegionAvail();
        if(v.x != static_cast<f32>(viewport_size_x) || v.y != static_cast<f32>(viewport_size_y)) {
            viewport_size_x = static_cast<u32>(v.x);
            viewport_size_y = static_cast<u32>(v.y);

            context.device.destroy_image(render_image);
            render_image = context.device.create_image({
                .format = daxa::Format::R8G8B8A8_UNORM,
                .aspect = daxa::ImageAspectFlagBits::COLOR,
                .size = { viewport_size_x, viewport_size_y, 1 },
                .usage = daxa::ImageUsageFlagBits::COLOR_ATTACHMENT | daxa::ImageUsageFlagBits::SHADER_READ_ONLY | daxa::ImageUsageFlagBits::TRANSFER_DST,
                .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
                .debug_name = "render_image"
            });

            context.device.destroy_image(depth_image);
            depth_image = context.device.create_image({
                .format = daxa::Format::D32_SFLOAT,
                .aspect = daxa::ImageAspectFlagBits::DEPTH,
                .size = { viewport_size_x, viewport_size_y, 1 },
                .usage = daxa::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT,
                .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
                .debug_name = "depth_image"
            });

            editor_camera.camera.resize(static_cast<i32>(v.x), static_cast<i32>(v.y));
        }

        ImGui::Image(*reinterpret_cast<ImTextureID const *>(&render_image), ImGui::GetContentRegionAvail(), ImVec2 {});
        ImGui::End();
        ImGui::PopStyleVar();

        ImGui::Render();
    }

    void Editor::render() {
        scene->update();

        daxa::ImageId swapchain_image = swapchain.acquire_next_image();
        if(swapchain_image.is_empty()) { return; }

        daxa::CommandList cmd_list = context.device.create_command_list({ .debug_name = "main loop cmd list" });

        editor_camera.camera.set_pos(editor_camera.position);
        editor_camera.camera.set_rot(editor_camera.rotation.x, editor_camera.rotation.y);
        editor_camera.update(deltaTime);

        glm::mat4 projection = editor_camera.camera.get_projection();
        glm::mat4 view = editor_camera.camera.get_view();

        glm::mat4 temp_inverse_projection_mat = glm::inverse(projection);
        glm::mat4 temp_inverse_view_mat = glm::inverse(view);

        CameraInfo camera_info = {
            .projection_matrix = *reinterpret_cast<f32mat4x4*>(&projection),
            .inverse_projection_matrix = *reinterpret_cast<f32mat4x4*>(&temp_inverse_projection_mat),
            .view_matrix = *reinterpret_cast<f32mat4x4*>(&view),
            .inverse_view_matrix = *reinterpret_cast<f32mat4x4*>(&temp_inverse_view_mat),
            .position = *reinterpret_cast<f32vec3*>(&editor_camera.position)
        };

        {
            auto* buffer_ptr = context.device.get_host_address_as<CameraInfo>(editor_camera_buffer);
            std::memcpy(buffer_ptr, &camera_info, sizeof(CameraInfo));
        }

        cmd_list.pipeline_barrier_image_transition({
            .waiting_pipeline_access = daxa::AccessConsts::TRANSFER_WRITE,
            .before_layout = daxa::ImageLayout::UNDEFINED,
            .after_layout = daxa::ImageLayout::ATTACHMENT_OPTIMAL,
            .image_id = render_image
        });

        cmd_list.begin_renderpass({
            .depth_attachment = {{
                .image_view = depth_image.default_view(),
                .load_op = daxa::AttachmentLoadOp::CLEAR,
                .clear_value = daxa::DepthValue{1.0f, 0},
            }},
            .render_area = {.x = 0, .y = 0, .width = viewport_size_x, .height = viewport_size_y},
        });
 
        cmd_list.set_pipeline(*depth_prepass_pipeline);

        scene->iterate([&](Entity entity){
            if(entity.has_component<ModelComponent>()) {
                auto& tc = entity.get_component<TransformComponent>();
                auto& mc = entity.get_component<ModelComponent>();
                if(!mc.model) { return; }

                DepthPrepassPush draw_push;
                //draw_push.mvp = *reinterpret_cast<daxa::types::f32mat4x4*>(&mvp);
                draw_push.camera_info = context.device.get_device_address(editor_camera_buffer);
                draw_push.transform_buffer = context.device.get_device_address(tc.transform_buffer);
                mc.model->draw(cmd_list, draw_push);
            }
        });

        cmd_list.end_renderpass();

        cmd_list.begin_renderpass({
            .color_attachments = {{
                .image_view = render_image.default_view(),
                .load_op = daxa::AttachmentLoadOp::CLEAR,
                .clear_value = std::array<daxa::f32, 4>{0.2f, 0.4f, 1.0f, 1.0f},
            }},
            .depth_attachment = {{
                .image_view = depth_image.default_view(),
                .load_op = daxa::AttachmentLoadOp::LOAD,
                .clear_value = daxa::DepthValue{1.0f, 0},
            }},
            .render_area = {.x = 0, .y = 0, .width = viewport_size_x, .height = viewport_size_y},
        });
 
        cmd_list.set_pipeline(*raster_pipeline);

        scene->iterate([&](Entity entity){
            if(entity.has_component<ModelComponent>()) {
                auto& tc = entity.get_component<TransformComponent>();
                auto& mc = entity.get_component<ModelComponent>();
                if(!mc.model) { return; }

                DrawPush draw_push;
                draw_push.camera_info = context.device.get_device_address(editor_camera_buffer);
                draw_push.transform_buffer = context.device.get_device_address(tc.transform_buffer);
                draw_push.light_buffer = context.device.get_device_address(scene->light_buffer);
                mc.model->draw(cmd_list, draw_push);
            }
        });

        cmd_list.end_renderpass();

        cmd_list.pipeline_barrier_image_transition({
            .waiting_pipeline_access = daxa::AccessConsts::TRANSFER_WRITE,
            .before_layout = daxa::ImageLayout::ATTACHMENT_OPTIMAL,
            .after_layout = daxa::ImageLayout::READ_ONLY_OPTIMAL,
            .image_id = render_image
        });

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
            window->width = static_cast<i32>(size_x);
            window->height = static_cast<i32>(size_y);

            render();
        //}
    }

    void Editor::on_key(i32 key, i32 action) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            if(toolbar_panel->scene_state == SceneState::Play) {
                window->set_mouse_capture(false);
            }
            toolbar_panel->scene_state = SceneState::Edit;
        }

        if (toolbar_panel->scene_state == SceneState::Play) {
            editor_camera.on_key(key, action);
        }
    }

    void Editor::on_mouse_move(f32 x, f32 y) {
        if (toolbar_panel->scene_state == SceneState::Play) {
            f32 center_x = static_cast<f32>(size_x / 2);
            f32 center_y = static_cast<f32>(size_y / 2);
            auto offset = glm::vec2{x - center_x, center_y - y};
            editor_camera.on_mouse_move(offset.x, offset.y);
            window->set_mouse_position(center_x, center_y);
        }
    }
}