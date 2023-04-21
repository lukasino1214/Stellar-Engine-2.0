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

        albedo_image = context.device.create_image({
            .format = daxa::Format::R8G8B8A8_SRGB,
            .aspect = daxa::ImageAspectFlagBits::COLOR,
            .size = { size_x, size_y, 1 },
            .usage = daxa::ImageUsageFlagBits::COLOR_ATTACHMENT | daxa::ImageUsageFlagBits::SHADER_READ_ONLY | daxa::ImageUsageFlagBits::TRANSFER_DST,
            .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
            .debug_name = "albedo_image"
        });

        normal_image = context.device.create_image({
            .format = daxa::Format::R16G16B16A16_SFLOAT,
            .aspect = daxa::ImageAspectFlagBits::COLOR,
            .size = { size_x, size_y, 1 },
            .usage = daxa::ImageUsageFlagBits::COLOR_ATTACHMENT | daxa::ImageUsageFlagBits::SHADER_READ_ONLY | daxa::ImageUsageFlagBits::TRANSFER_DST,
            .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
            .debug_name = "normal_image"
        });

        render_image = context.device.create_image({
            .format = daxa::Format::R8G8B8A8_SRGB,
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

        this->ssao_image = context.device.create_image({
            .format = daxa::Format::R8_UNORM,
            .aspect = daxa::ImageAspectFlagBits::COLOR,
            .size = { size_x / 2, size_y / 2, 1 },
            .usage = daxa::ImageUsageFlagBits::COLOR_ATTACHMENT | daxa::ImageUsageFlagBits::SHADER_READ_ONLY,
            .debug_name = "ssao_image"
        });

        this->ssao_blur_image = context.device.create_image({
            .format = daxa::Format::R8_UNORM,
            .aspect = daxa::ImageAspectFlagBits::COLOR,
            .size = { size_x / 2, size_y / 2, 1 },
            .usage = daxa::ImageUsageFlagBits::COLOR_ATTACHMENT | daxa::ImageUsageFlagBits::SHADER_READ_ONLY,
            .debug_name = "ssao_blur_image"
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
            .debug_name = "depth_prepass_pipeline",
        }).value();

        deffered_pipeline = context.pipeline_manager.add_raster_pipeline({
            .vertex_shader_info = {.source = daxa::ShaderFile{"deffered.glsl"}, .compile_options = {.defines = {daxa::ShaderDefine{"DRAW_VERT"}}}},
            .fragment_shader_info = {.source = daxa::ShaderFile{"deffered.glsl"}, .compile_options = {.defines = {daxa::ShaderDefine{"DRAW_FRAG"}}}},
            .color_attachments = {
                { .format = daxa::Format::R8G8B8A8_SRGB },
                { .format = daxa::Format::R16G16B16A16_SFLOAT }
            },
            .depth_test = {
                .depth_attachment_format = daxa::Format::D32_SFLOAT,
                .enable_depth_test = true,
                .enable_depth_write = false,
            },
            .raster = {
                .face_culling = daxa::FaceCullFlagBits::FRONT_BIT
            },
            .push_constant_size = sizeof(DrawPush),
            .debug_name = "deffered_pipeline",
        }).value();

        composition_pipeline = context.pipeline_manager.add_raster_pipeline({
            .vertex_shader_info = {.source = daxa::ShaderFile{"composition.glsl"}, .compile_options = {.defines = {daxa::ShaderDefine{"DRAW_VERT"}}}},
            .fragment_shader_info = {.source = daxa::ShaderFile{"composition.glsl"}, .compile_options = {.defines = {daxa::ShaderDefine{"DRAW_FRAG"}}}},
            .color_attachments = {
                { .format = daxa::Format::R8G8B8A8_SRGB },
            },
            .raster = {
                .face_culling = daxa::FaceCullFlagBits::NONE
            },
            .push_constant_size = sizeof(CompositionPush),
            .debug_name = "composition_pipeline",
        }).value();

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

        this->ssao_generation_pipeline = context.pipeline_manager.add_raster_pipeline({
            .vertex_shader_info = { .source = daxa::ShaderFile{"ssao_generation.glsl"}, .compile_options = { .defines = { daxa::ShaderDefine{"DRAW_VERT"}}}},
            .fragment_shader_info = { .source = daxa::ShaderFile{"ssao_generation.glsl"}, .compile_options = { .defines = { daxa::ShaderDefine{"DRAW_FRAG"}}}},
            .color_attachments = { { .format = daxa::Format::R8_UNORM, } },
            .raster = { .polygon_mode = daxa::PolygonMode::FILL, },
            .push_constant_size = sizeof(SSAOGenerationPush),
            .debug_name = "ssao_generation_code",
        }).value();

        this->ssao_blur_pipeline = context.pipeline_manager.add_raster_pipeline({
            .vertex_shader_info = { .source = daxa::ShaderFile{"ssao_blur.glsl"}, .compile_options = { .defines = { daxa::ShaderDefine{"DRAW_VERT"}}}},
            .fragment_shader_info = { .source = daxa::ShaderFile{"ssao_blur.glsl"}, .compile_options = { .defines = { daxa::ShaderDefine{"DRAW_FRAG"}}}},
            .color_attachments = { { .format = daxa::Format::R8_UNORM, } },
            .raster = { .polygon_mode = daxa::PolygonMode::FILL, },
            .push_constant_size = sizeof(SSAOBlurPush),
            .debug_name = "ssao_blur_pipeline",
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

        std::vector<f32> vertices = {
            -0.5f, -0.5f, -0.5f,
            0.5f, -0.5f, -0.5f,
            0.5f,  0.5f, -0.5f,
            0.5f,  0.5f, -0.5f,
            -0.5f,  0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,

            -0.5f, -0.5f,  0.5f,
            0.5f, -0.5f,  0.5f,
            0.5f,  0.5f,  0.5f,
            0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,
            -0.5f, -0.5f,  0.5f,

            -0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,
            -0.5f, -0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,

            0.5f,  0.5f,  0.5f,
            0.5f,  0.5f, -0.5f,
            0.5f, -0.5f, -0.5f,
            0.5f, -0.5f, -0.5f,
            0.5f, -0.5f,  0.5f,
            0.5f,  0.5f,  0.5f,

            -0.5f, -0.5f, -0.5f,
            0.5f, -0.5f, -0.5f,
            0.5f, -0.5f,  0.5f,
            0.5f, -0.5f,  0.5f,
            -0.5f, -0.5f,  0.5f,
            -0.5f, -0.5f, -0.5f,

            -0.5f,  0.5f, -0.5f,
            0.5f,  0.5f, -0.5f,
            0.5f,  0.5f,  0.5f,
            0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f, -0.5f,
        };

        daxa::BufferId temp_face_buffer = context.device.create_buffer({
            .memory_flags = daxa::MemoryFlagBits::HOST_ACCESS_RANDOM,
            .size = static_cast<u32>(sizeof(f32) * vertices.size()),
        });

        face_buffer = context.device.create_buffer({
            .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
            .size = static_cast<u32>(sizeof(f32) * vertices.size()),
        });

        std::memcpy(context.device.get_host_address_as<f32>(temp_face_buffer), vertices.data(), sizeof(f32) * vertices.size());

        auto cmd_list = context.device.create_command_list({
            .debug_name = "",
        });

        cmd_list.destroy_buffer_deferred(temp_face_buffer);

        cmd_list.copy_buffer_to_buffer({
            .src_buffer = temp_face_buffer,
            .dst_buffer = face_buffer,
            .size = static_cast<u32>(sizeof(f32) * vertices.size()),
        });

        cmd_list.complete();
        context.device.submit_commands({
            .command_lists = {std::move(cmd_list)},
        });

        /*{
            Entity eaa = scene->create_entity("bruh");
            auto m = std::make_shared<Model>(context.device, "models/glTF/BarramundiFish.gltf");
            for(auto& p : m->primitives) {
                std::cout << p.index_count << std::endl;
            }
            auto& mc = eaa.add_component<ModelComponent>();
            mc.model = m;
            eaa.add_component<TransformComponent>();
        }*/

        /*std::cout << "----" << std::endl;

        {
            Entity eaa = scene->create_entity("bruh 2");
            auto m = std::make_shared<Model>(context.device, "models/FlightHelmet/glTF/FlightHelmet.gltf");
            
            for(auto& p : m->primitives) {
                std::cout << p.index_count << std::endl;
            }
            auto& mc = eaa.add_component<ModelComponent>();
            mc.model = m;
            auto& tc = eaa.add_component<TransformComponent>();
            tc.position.x = 2.0f;
        }

        std::cout << "----" << std::endl;

        {
            Entity eaa = scene->create_entity("bruh 2");
            auto m = std::make_shared<Model>(context.device, "models/FlightHelmet/glTF/FlightHelmet.gltf");
            
            for(auto& p : m->primitives) {
                std::cout << p.index_count << std::endl;
            }
            auto& mc = eaa.add_component<ModelComponent>();
            mc.model = m;
            auto& tc = eaa.add_component<TransformComponent>();
            tc.position.x = 2.0f;
        }

        std::cout << "----" << std::endl;

        {
            Entity eaa = scene->create_entity("bruh 3");
            auto m = std::make_shared<Model>(context.device, "models/ABeautifulGame/glTF/ABeautifulGame.gltf");
            
            for(auto& p : m->primitives) {
                std::cout << p.index_count << std::endl;
            }
            auto& mc = eaa.add_component<ModelComponent>();
            mc.model = m;
            auto& tc = eaa.add_component<TransformComponent>();
            tc.position.x = 4.0f;
        }*/

    }

    Editor::~Editor() {
        ImGui_ImplGlfw_Shutdown();

        context.device.wait_idle();
        context.device.collect_garbage();
        context.device.destroy_sampler(sampler);
        context.device.destroy_image(albedo_image);
        context.device.destroy_image(normal_image);
        context.device.destroy_image(render_image);
        context.device.destroy_image(depth_image);
        context.device.destroy_image(ssao_image);
        context.device.destroy_image(ssao_blur_image);
        context.device.destroy_buffer(editor_camera_buffer);
        context.device.destroy_buffer(face_buffer);
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

        viewport_panel->draw(displayed_image.is_empty() ? render_image : displayed_image, window, scene_hiearchy_panel, editor_camera);
        if(viewport_panel->should_resize) {
            context.device.destroy_image(albedo_image);
            albedo_image = context.device.create_image({
                .format = daxa::Format::R8G8B8A8_SRGB,
                .aspect = daxa::ImageAspectFlagBits::COLOR,
                .size = { viewport_panel->viewport_size_x, viewport_panel->viewport_size_y, 1 },
                .usage = daxa::ImageUsageFlagBits::COLOR_ATTACHMENT | daxa::ImageUsageFlagBits::SHADER_READ_ONLY | daxa::ImageUsageFlagBits::TRANSFER_DST,
                .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
                .debug_name = "albedo_image"
            });

            context.device.destroy_image(normal_image);
            normal_image = context.device.create_image({
                .format = daxa::Format::R16G16B16A16_SFLOAT,
                .aspect = daxa::ImageAspectFlagBits::COLOR,
                .size = { viewport_panel->viewport_size_x, viewport_panel->viewport_size_y, 1 },
                .usage = daxa::ImageUsageFlagBits::COLOR_ATTACHMENT | daxa::ImageUsageFlagBits::SHADER_READ_ONLY | daxa::ImageUsageFlagBits::TRANSFER_DST,
                .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
                .debug_name = "normal_image"
            });

            context.device.destroy_image(render_image);
            render_image = context.device.create_image({
                .format = daxa::Format::R8G8B8A8_SRGB,
                .aspect = daxa::ImageAspectFlagBits::COLOR,
                .size = { viewport_panel->viewport_size_x, viewport_panel->viewport_size_y, 1 },
                .usage = daxa::ImageUsageFlagBits::COLOR_ATTACHMENT | daxa::ImageUsageFlagBits::SHADER_READ_ONLY | daxa::ImageUsageFlagBits::TRANSFER_DST,
                .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
                .debug_name = "render_image"
            });

            context.device.destroy_image(depth_image);
            depth_image = context.device.create_image({
                .format = daxa::Format::D32_SFLOAT,
                .aspect = daxa::ImageAspectFlagBits::DEPTH,
                .size = { viewport_panel->viewport_size_x, viewport_panel->viewport_size_y, 1 },
                .usage = daxa::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT | daxa::ImageUsageFlagBits::SHADER_READ_ONLY,
                .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
                .debug_name = "depth_image"
            });

            context.device.destroy_image(ssao_image);
            this->ssao_image = context.device.create_image({
                .format = daxa::Format::R8_UNORM,
                .aspect = daxa::ImageAspectFlagBits::COLOR,
                .size = { viewport_panel->viewport_size_x / 2, viewport_panel->viewport_size_y / 2, 1 },
                .usage = daxa::ImageUsageFlagBits::COLOR_ATTACHMENT | daxa::ImageUsageFlagBits::SHADER_READ_ONLY,
                .debug_name = "ssao_image"
            });

            context.device.destroy_image(ssao_blur_image);
            this->ssao_blur_image = context.device.create_image({
                .format = daxa::Format::R8_UNORM,
                .aspect = daxa::ImageAspectFlagBits::COLOR,
                .size = { viewport_panel->viewport_size_x / 2, viewport_panel->viewport_size_y / 2, 1 },
                .usage = daxa::ImageUsageFlagBits::COLOR_ATTACHMENT | daxa::ImageUsageFlagBits::SHADER_READ_ONLY,
                .debug_name = "ssao_blur_image"
            });

            viewport_panel->should_resize = false;
            displayed_image = render_image;
        }

        ImGui::Begin("G-Buffer Attachments");
        if(ImGui::Checkbox("Render image", &show_render)) { displayed_image = render_image; }
        if(ImGui::Checkbox("Albedo image", &show_albedo)) { displayed_image = albedo_image; }
        if(ImGui::Checkbox("Normal image", &show_normal)) { displayed_image = normal_image; }
        if(ImGui::Checkbox("SSAO image", &show_ssao)) { displayed_image = ssao_blur_image; }
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

        cmd_list.pipeline_barrier_image_transition({
            .waiting_pipeline_access = daxa::AccessConsts::TRANSFER_WRITE,
            .before_layout = daxa::ImageLayout::UNDEFINED,
            .after_layout = daxa::ImageLayout::ATTACHMENT_OPTIMAL,
            .image_id = render_image
        });

        cmd_list.pipeline_barrier_image_transition({
            .waiting_pipeline_access = daxa::AccessConsts::TRANSFER_WRITE,
            .before_layout = daxa::ImageLayout::UNDEFINED,
            .after_layout = daxa::ImageLayout::GENERAL,
            .image_slice = {.image_aspect = daxa::ImageAspectFlagBits::DEPTH },
            .image_id = depth_image,
        });

        cmd_list.pipeline_barrier_image_transition({
            .waiting_pipeline_access = daxa::AccessConsts::TRANSFER_WRITE,
            .before_layout = daxa::ImageLayout::UNDEFINED,
            .after_layout = daxa::ImageLayout::ATTACHMENT_OPTIMAL,
            .image_id = albedo_image
        });

        cmd_list.pipeline_barrier_image_transition({
            .waiting_pipeline_access = daxa::AccessConsts::TRANSFER_WRITE,
            .before_layout = daxa::ImageLayout::UNDEFINED,
            .after_layout = daxa::ImageLayout::ATTACHMENT_OPTIMAL,
            .image_id = normal_image
        });

        cmd_list.pipeline_barrier_image_transition({
            .waiting_pipeline_access = daxa::AccessConsts::TRANSFER_WRITE,
            .before_layout = daxa::ImageLayout::UNDEFINED,
            .after_layout = daxa::ImageLayout::ATTACHMENT_OPTIMAL,
            .image_id = ssao_image
        });

        cmd_list.pipeline_barrier_image_transition({
            .waiting_pipeline_access = daxa::AccessConsts::TRANSFER_WRITE,
            .before_layout = daxa::ImageLayout::UNDEFINED,
            .after_layout = daxa::ImageLayout::ATTACHMENT_OPTIMAL,
            .image_id = ssao_blur_image
        });

        cmd_list.begin_renderpass({
            .depth_attachment = {{
                .image_view = depth_image.default_view(),
                .load_op = daxa::AttachmentLoadOp::CLEAR,
                .clear_value = daxa::DepthValue{1.0f, 0},
            }},
            .render_area = {.x = 0, .y = 0, .width = viewport_panel->viewport_size_x, .height = viewport_panel->viewport_size_y},
        });
 
        cmd_list.set_pipeline(*depth_prepass_pipeline);

        scene->iterate([&](Entity entity){
            if(entity.has_component<ModelComponent>()) {
                auto& tc = entity.get_component<TransformComponent>();
                auto& mc = entity.get_component<ModelComponent>();
                if(!mc.model) { return; }

                DepthPrepassPush draw_push;
                draw_push.camera_info = context.device.get_device_address(editor_camera_buffer);
                draw_push.transform_buffer = context.device.get_device_address(tc.transform_buffer);
                mc.model->draw(cmd_list, draw_push);
            }
        });

        cmd_list.end_renderpass();



        cmd_list.begin_renderpass({
            .color_attachments = {
                {
                    .image_view = this->albedo_image.default_view(),
                    .load_op = daxa::AttachmentLoadOp::CLEAR,
                    .clear_value = std::array<f32, 4>{0.05f, 0.05f, 0.05f, 1.0f},
                },
                {
                    .image_view = this->normal_image.default_view(),
                    .load_op = daxa::AttachmentLoadOp::CLEAR,
                    .clear_value = std::array<f32, 4>{0.0f, 0.0f, 0.0f, 1.0f},
                },
            },
            .depth_attachment = {{
                .image_view = depth_image.default_view(),
                .load_op = daxa::AttachmentLoadOp::LOAD,
                .clear_value = daxa::DepthValue{1.0f, 0},
            }},
            .render_area = {.x = 0, .y = 0, .width = viewport_panel->viewport_size_x, .height = viewport_panel->viewport_size_y},
        });
 
        cmd_list.set_pipeline(*deffered_pipeline);

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

        cmd_list.begin_renderpass({
            .color_attachments = {{
                    .image_view = this->ssao_image.default_view(),
                    .load_op = daxa::AttachmentLoadOp::CLEAR,
                    .clear_value = std::array<f32, 4>{1.0f, 0.0f, 0.0f, 1.0f},
            }},
            .render_area = {.x = 0, .y = 0, .width = viewport_panel->viewport_size_x / 2, .height = viewport_panel->viewport_size_y / 2},
        });
        cmd_list.set_pipeline(*ssao_generation_pipeline);
        cmd_list.push_constant(SSAOGenerationPush {
            .normal = { .texture_id = normal_image.default_view(), .sampler_id = sampler },
            .depth = { .texture_id = depth_image.default_view(), .sampler_id = sampler },
            .camera_info = context.device.get_device_address(editor_camera_buffer)
        });
        cmd_list.draw({ .vertex_count = 3 });
        cmd_list.end_renderpass();
    // SSAO blur
        cmd_list.begin_renderpass({
            .color_attachments = {
                {
                    .image_view = this->ssao_blur_image.default_view(),
                    .load_op = daxa::AttachmentLoadOp::CLEAR,
                    .clear_value = std::array<f32, 4>{0.0f, 0.0f, 0.0f, 1.0f},
                },
            },
            .render_area = {.x = 0, .y = 0, .width = viewport_panel->viewport_size_x / 2, .height = viewport_panel->viewport_size_y / 2},
        });
        cmd_list.set_pipeline(*ssao_blur_pipeline);
        cmd_list.push_constant(SSAOBlurPush {
            .ssao = { .texture_id = ssao_image.default_view(), .sampler_id = sampler },
        });
        cmd_list.draw({ .vertex_count = 3 });
        cmd_list.end_renderpass();

        glm::vec3 dir = { 0.0f, -1.0f, 0.0f };
        dir = glm::rotateZ(dir, upTime / sun_factor);

        cmd_list.begin_renderpass({
            .color_attachments = {
                {
                    .image_view = this->render_image.default_view(),
                    .load_op = daxa::AttachmentLoadOp::CLEAR,
                    .clear_value = std::array<f32, 4>{0.05f, 0.05f, 0.05f, 1.0f},
                },
            },
            .render_area = {.x = 0, .y = 0, .width = viewport_panel->viewport_size_x, .height = viewport_panel->viewport_size_y},
        });

        cmd_list.set_pipeline(*composition_pipeline);
        cmd_list.push_constant(CompositionPush {
            .albedo = { .texture_id = albedo_image.default_view(), .sampler_id = sampler },
            .normal = { .texture_id = normal_image.default_view(), .sampler_id = sampler },
            .depth = { .texture_id = depth_image.default_view(), .sampler_id = sampler },
            .ssao = { .texture_id = ssao_blur_image.default_view(), .sampler_id = sampler },
            .light_buffer = context.device.get_device_address(scene->light_buffer),
            .camera_info = context.device.get_device_address(editor_camera_buffer),
            .ambient = glm::dot(dir, {0.0f, -1.0f, 0.0f})
        });

        cmd_list.draw({ .vertex_count = 3 });
        cmd_list.end_renderpass();


        cmd_list.begin_renderpass({
            .color_attachments = {{
                .image_view = render_image.default_view(),
                .load_op = daxa::AttachmentLoadOp::LOAD,
                .clear_value = std::array<daxa::f32, 4>{0.2f, 0.4f, 1.0f, 1.0f},
            }},
            .depth_attachment = {{
                .image_view = depth_image.default_view(),
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

        glm::mat4 model_matrix = glm::translate(glm::mat4{1.0f}, glm::vec3{ 0.0f, 0.0f, 0.0f }) * glm::scale(glm::mat4{1.0f}, glm::vec3{ 1000.0f, 1000.0f, 1000.0f });

        glm::vec3 sun_position = glm::vec3{ 0.0f, 6372e3f, 0.0f };
        sun_position = glm::rotateZ(sun_position, upTime / sun_factor);
        cmd_list.set_pipeline(*atmosphere_pipeline);
        cmd_list.push_constant(SkyPush {
            .vertex_buffer = context.device.get_device_address(face_buffer),
            .model_matrix = *reinterpret_cast<f32mat4x4*>(&model_matrix),
            .sun_position = *reinterpret_cast<f32vec3*>(&sun_position),
            .camera_info = context.device.get_device_address(editor_camera_buffer)
        });
        cmd_list.draw({ .vertex_count = 36 });

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