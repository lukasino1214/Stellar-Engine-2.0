#include "editor.hpp"
#include <core/types.hpp>
#include <data/components.hpp>

#include <daxa/types.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/trigonometric.hpp>
#include <thread>

#include <imgui.h>
#include <imgui_impl_glfw.h>

#include <data/scene.hpp>
#include <data/entity.hpp>
#include <core/logger.hpp>

#include "../shaders/shared.inl"

namespace Stellar {
    static constexpr f32 sun_factor = 64.0f;

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
        
        scene = std::make_shared<Scene>("Test", context.device, context.pipeline_manager);
        scene->deserialize("test.scene");

        scene_hiearchy_panel = std::make_unique<SceneHiearchyPanel>(scene);
        asset_browser_panel = std::make_unique<AssetBrowserPanel>(project_path);
        toolbar_panel = std::make_unique<ToolbarPanel>(window);
        performance_stats_panel = std::make_unique<PerformanceStatsPanel>();
        logger_panel = std::make_unique<LoggerPanel>();
        viewport_panel = std::make_unique<ViewportPanel>();

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
        glfwSetMouseButtonCallback(window->glfw_window_ptr, [](GLFWwindow * window_ptr, i32 button, i32 action, i32) {
            auto & app = *reinterpret_cast<Editor *>(glfwGetWindowUserPointer(window_ptr));
            app.on_mouse_button(button, action);
        });

        ImGui_ImplGlfw_InitForVulkan(window->glfw_window_ptr, true);
        imgui_renderer =  daxa::ImGuiRenderer({
            .device = context.device,
            .format = swapchain.get_format(),
        });
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        size_x = swapchain.get_surface_extent().x;
        size_y = swapchain.get_surface_extent().y;

        Logger::init();
        CORE_INFO("Test");

        billboard_pipeline = context.pipeline_manager.add_raster_pipeline({
            .vertex_shader_info = {.source = daxa::ShaderFile{"billboard.glsl"}, .compile_options = {.defines = {daxa::ShaderDefine{"DRAW_VERT"}}}},
            .fragment_shader_info = {.source = daxa::ShaderFile{"billboard.glsl"}, .compile_options = {.defines = {daxa::ShaderDefine{"DRAW_FRAG"}}}},
            .color_attachments = {{ .format = daxa::Format::R8G8B8A8_SRGB }},
            .depth_test = {
                .depth_attachment_format = daxa::Format::D32_SFLOAT,
                .enable_depth_test = true,
                .enable_depth_write = true,
            },
            .raster = {
                .face_culling = daxa::FaceCullFlagBits::NONE
            },
            .push_constant_size = sizeof(BillboardPush),
            .debug_name = "billboard_pipeline",
        }).value();

        lines_pipeline = context.pipeline_manager.add_raster_pipeline({
            .vertex_shader_info = {.source = daxa::ShaderFile{"lines.glsl"}, .compile_options = {.defines = {daxa::ShaderDefine{"DRAW_VERT"}}}},
            .fragment_shader_info = {.source = daxa::ShaderFile{"lines.glsl"}, .compile_options = {.defines = {daxa::ShaderDefine{"DRAW_FRAG"}}}},
            .color_attachments = {{ .format = daxa::Format::R8G8B8A8_SRGB }},
            .depth_test = {
                .depth_attachment_format = daxa::Format::D32_SFLOAT,
                .enable_depth_test = true,
                .enable_depth_write = true,
            },
            .raster = {
                .primitive_topology = daxa::PrimitiveTopology::LINE_LIST,
                .polygon_mode = daxa::PolygonMode::LINE,
                .face_culling = daxa::FaceCullFlagBits::NONE
            },
            .push_constant_size = sizeof(LinesPush),
            .debug_name = "lines_pipeline",
        }).value();

        atmosphere_pipeline = context.pipeline_manager.add_raster_pipeline({
            .vertex_shader_info = {.source = daxa::ShaderFile{"atmosphere.glsl"}, .compile_options = {.defines = {daxa::ShaderDefine{"DRAW_VERT"}}}},
            .fragment_shader_info = {.source = daxa::ShaderFile{"atmosphere.glsl"}, .compile_options = {.defines = {daxa::ShaderDefine{"DRAW_FRAG"}}}},
            .color_attachments = {{ .format = daxa::Format::R8G8B8A8_SRGB }},
            .depth_test = {
                .depth_attachment_format = daxa::Format::D32_SFLOAT,
                .enable_depth_test = true,
                .enable_depth_write = true,
            },
            .raster = {
                .face_culling = daxa::FaceCullFlagBits::NONE
            },
            .push_constant_size = sizeof(SkyPush),
            .debug_name = "atmosphere_pipeline",
        }).value();

        deffered_rendering_system = std::make_unique<DefferedRenderingSystem>(context.device, context.pipeline_manager);
        ssao_system = std::make_unique<SSAOSystem>(context.device, context.pipeline_manager);

        editor_camera.camera.resize(static_cast<i32>(size_x), static_cast<i32>(size_y));

        Entity e = scene->create_entity("Directional Light");
        e.add_component<TransformComponent>();
        e.add_component<DirectionalLightComponent>();

        editor_camera_buffer = context.device.create_buffer({
            .memory_flags = daxa::MemoryFlagBits::HOST_ACCESS_RANDOM,
            .size = sizeof(CameraInfo),
            .debug_name = "editor camera buffer"
        });

        directional_light_texture = std::make_unique<Texture>(context.device, "directional_light_icon.png", daxa::Format::R8G8B8A8_SRGB);
        point_light_texture = std::make_unique<Texture>(context.device, "point_light_icon.png", daxa::Format::R8G8B8A8_SRGB);
        spot_light_texture = std::make_unique<Texture>(context.device, "spot_light_icon.png", daxa::Format::R8G8B8A8_SRGB);
    
        sampler = context.device.create_sampler({
            .magnification_filter = daxa::Filter::LINEAR,
            .minification_filter = daxa::Filter::LINEAR,
            .mipmap_filter = daxa::Filter::LINEAR,
            .address_mode_u = daxa::SamplerAddressMode::CLAMP_TO_EDGE,
            .address_mode_v = daxa::SamplerAddressMode::CLAMP_TO_EDGE,
            .address_mode_w = daxa::SamplerAddressMode::CLAMP_TO_EDGE,
            .mip_lod_bias = 0.0f,
            .enable_anisotropy = true,
            .max_anisotropy = 16.0f,
            .enable_compare = false,
            .compare_op = daxa::CompareOp::ALWAYS,
            .min_lod = 0.0f,
            .max_lod = static_cast<f32>(1),
            .enable_unnormalized_coordinates = false,
        });

        cube_model = std::make_unique<Model>(context.device, "models/cube.gltf");
    }

    Editor::~Editor() {
        ImGui_ImplGlfw_Shutdown();

        context.device.wait_idle();
        context.device.collect_garbage();
        context.device.destroy_sampler(sampler);
        context.device.destroy_buffer(editor_camera_buffer);
    }

    void Editor::run() {
        while(!window->should_close()) {
            glfwPollEvents();

            f64 currentFrameTime = glfwGetTime();

            deltaTime = static_cast<f32>(currentFrameTime - lastFrameTime);
            lastFrameTime = currentFrameTime;

            upTime = static_cast<f32>(currentFrameTime);
            FPS = static_cast<u32>(glm::ceil(1.0f / deltaTime));

            ui_update();

            if(toolbar_panel->play && deltaTime > 0.0f) {
                scene->physics_update(deltaTime);
            }

            render();
        }
    }

    void Editor::ui_update() {
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

        viewport_panel->draw(displayed_image.is_empty() ? deffered_rendering_system->render_image : displayed_image, window, scene_hiearchy_panel, editor_camera);
        if(viewport_panel->should_resize) {
            deffered_rendering_system->resize(viewport_panel->viewport_size_x, viewport_panel->viewport_size_y);
            ssao_system->resize(viewport_panel->viewport_size_x, viewport_panel->viewport_size_y);;

            viewport_panel->should_resize = false;
            displayed_image = deffered_rendering_system->render_image;
        }

        ImGui::Begin("G-Buffer Attachments");
        if(ImGui::Checkbox("Render image", &show_render)) { displayed_image = deffered_rendering_system->render_image; }
        if(ImGui::Checkbox("Albedo image", &show_albedo)) { displayed_image = deffered_rendering_system->albedo_image; }
        if(ImGui::Checkbox("Normal image", &show_normal)) { displayed_image = deffered_rendering_system->normal_image; }
        if(ImGui::Checkbox("SSAO image", &show_ssao)) { displayed_image = ssao_system->ssao_blur_image; }
        ImGui::End();

        ImGui::Begin("TEST");

        if(ImGui::Button("save")) {
            scene->serialize("test_scene_2.scene");
        }

        ImGui::End();

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

        deffered_rendering_system->render_gbuffer(cmd_list, DefferedRenderingSystem::DefferedRenderInfo {
            .scene = scene,
            .camera_buffer_address = context.device.get_device_address(editor_camera_buffer)
        });

        ssao_system->render(cmd_list, SSAOSystem::RenderInfo {
            .depth_image = deffered_rendering_system->depth_image,
            .normal_image = deffered_rendering_system->normal_image,
            .sampler =sampler,
            .camera_buffer_address = context.device.get_device_address(editor_camera_buffer)
        });

        glm::vec3 dir = { 0.0f, -1.0f, 0.0f };
        dir = glm::rotateZ(dir, upTime / sun_factor);

        deffered_rendering_system->render_composition(cmd_list, DefferedRenderingSystem::CompositionRenderInfo{
            .scene = scene,
            .sampler = sampler,
            .ssao_image = ssao_system->ssao_blur_image,
            .camera_buffer_address = context.device.get_device_address(editor_camera_buffer),
            .ambient = glm::dot(dir, {0.0f, -1.0f, 0.0f})
        });

        cmd_list.begin_renderpass({
            .color_attachments = {{
                .image_view = deffered_rendering_system->render_image.default_view(),
                .load_op = daxa::AttachmentLoadOp::LOAD,
                .clear_value = std::array<daxa::f32, 4>{0.2f, 0.4f, 1.0f, 1.0f},
            }},
            .depth_attachment = {{
                .image_view = deffered_rendering_system->depth_image.default_view(),
                .load_op = daxa::AttachmentLoadOp::LOAD,
                .clear_value = daxa::DepthValue{1.0f, 0},
            }},
            .render_area = {.x = 0, .y = 0, .width = viewport_panel->viewport_size_x, .height = viewport_panel->viewport_size_y},
        });

        cmd_list.set_pipeline(*billboard_pipeline); 
        
        scene->iterate([&](Entity entity){
            daxa::ImageId image_id;
            daxa::SamplerId sampler_id;
            bool is_light = false;
            
            if(entity.has_component<DirectionalLightComponent>()) {
                image_id = directional_light_texture->image_id;
                sampler_id = directional_light_texture->sampler_id;
                is_light = true;
            }

            if(entity.has_component<PointLightComponent>()) {
                image_id = point_light_texture->image_id;
                sampler_id = point_light_texture->sampler_id;
                is_light = true;
            }

            if(entity.has_component<SpotLightComponent>()) {
                image_id = spot_light_texture->image_id;
                sampler_id = spot_light_texture->sampler_id;
                is_light = true;
            }

            if(!is_light) { return; }

            auto& tc = entity.get_component<TransformComponent>();
            cmd_list.push_constant(BillboardPush {
                .position = *reinterpret_cast<f32vec3*>(&tc.position),
                .camera_info = context.device.get_device_address(editor_camera_buffer),
                .texture = {
                    .texture_id = image_id.default_view(),
                    .sampler_id = sampler_id
                }
            });

            cmd_list.draw({ .vertex_count = 6 });
        });

        cmd_list.set_pipeline(*lines_pipeline);
        cmd_list.push_constant(LinesPush {
            .vertex_buffer = context.device.get_device_address(scene->lines_buffer),
            .camera_info = context.device.get_device_address(editor_camera_buffer),
        });
        cmd_list.draw({ .vertex_count = scene->lines_vertices });

        glm::mat4 model_matrix = glm::translate(glm::mat4{1.0f}, glm::vec3{ 0.0f, 0.0f, 0.0f }) * glm::scale(glm::mat4{1.0f}, glm::vec3{ 1000.0f, 1000.0f, 1000.0f });

        glm::vec3 sun_position = glm::vec3{ 0.0f, 6372e3f, 0.0f };
        sun_position = glm::rotateZ(sun_position, upTime / sun_factor);
        cmd_list.set_pipeline(*atmosphere_pipeline);
        SkyPush sky_push;
        sky_push.model_matrix = *reinterpret_cast<f32mat4x4*>(&model_matrix);
        sky_push.sun_position = *reinterpret_cast<f32vec3*>(&sun_position);
        sky_push.camera_info = context.device.get_device_address(editor_camera_buffer);
        cube_model->draw(cmd_list, sky_push);
        cmd_list.end_renderpass();

        cmd_list.pipeline_barrier_image_transition({
            .waiting_pipeline_access = daxa::AccessConsts::TRANSFER_WRITE,
            .before_layout = daxa::ImageLayout::ATTACHMENT_OPTIMAL,
            .after_layout = daxa::ImageLayout::READ_ONLY_OPTIMAL,
            .image_id = deffered_rendering_system->render_image
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

    void Editor::on_resize(u32, u32) {
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
        if (viewport_panel->should_play) {
            editor_camera.on_key(key, action);
        }
    }

    void Editor::on_mouse_move(f32 x, f32 y) {
        viewport_panel->mouse_pos = { x, y };
    }

    void Editor::on_mouse_button(i32 button, i32 action) {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            viewport_panel->mouse_pressed = true;
        } else {
            viewport_panel->mouse_pressed = false;
        }
    }
}