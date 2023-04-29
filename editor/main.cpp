#include <chrono>
#include <iostream>

#include <daxa/daxa.hpp>
#include <daxa/utils/pipeline_manager.hpp>
#include <daxa/utils/imgui.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <stdexcept>
#include <string_view>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <graphics/texture.hpp>
#include <core/window.hpp>


#include "../shaders/shared.inl"

#include <thread>
using namespace std::literals;

#include "editor.hpp"

auto loading_screen(Stellar::Context& context, daxa::Swapchain swapchain) -> bool {
    Stellar::Texture texture = Stellar::Texture(context.device, "pic.png", daxa::Format::R8G8B8A8_SRGB);

    auto pipeline = context.pipeline_manager.add_raster_pipeline({
        .vertex_shader_info = {.source = daxa::ShaderFile{"loading_screen.glsl"}, .compile_options = {.defines = {daxa::ShaderDefine{"DRAW_VERT"}}}},
        .fragment_shader_info = {.source = daxa::ShaderFile{"loading_screen.glsl"}, .compile_options = {.defines = {daxa::ShaderDefine{"DRAW_FRAG"}}}},
        .color_attachments = {{ .format = swapchain.get_format() }},
        .depth_test = {
            .depth_attachment_format = daxa::Format::D32_SFLOAT,
            .enable_depth_test = true,
            .enable_depth_write = true,
        },
        .raster = {
            .face_culling = daxa::FaceCullFlagBits::NONE
        },
        .push_constant_size = sizeof(TexturePush),
        .debug_name = "raster_pipeline",
    }).value();

    auto timer = std::chrono::system_clock::now();

    while(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - timer).count() < 500) {
        glfwPollEvents();

        daxa::ImageId swapchain_image = swapchain.acquire_next_image();
        if(swapchain_image.is_empty()) { continue; }

        daxa::CommandList cmd_list = context.device.create_command_list({ .debug_name = "main loop cmd list" });

        cmd_list.pipeline_barrier_image_transition({
            .waiting_pipeline_access = daxa::AccessConsts::TRANSFER_WRITE,
            .before_layout = daxa::ImageLayout::UNDEFINED,
            .after_layout = daxa::ImageLayout::TRANSFER_DST_OPTIMAL,
            .image_id = swapchain_image
        });

        cmd_list.begin_renderpass({
            .color_attachments = {{
                .image_view = swapchain_image.default_view(),
                .load_op = daxa::AttachmentLoadOp::CLEAR,
                .clear_value = std::array<daxa::f32, 4>{0.2f, 0.4f, 1.0f, 1.0f},
            }},
            .render_area = {.x = 0, .y = 0, .width = 520, .height = 220},
        });

        cmd_list.set_pipeline(*pipeline);


        cmd_list.push_constant(TexturePush {
            .texture = texture.image_id.default_view(),
            .texture_sampler = texture.sampler_id
        });
        cmd_list.draw({ .vertex_count = 3 });
        cmd_list.end_renderpass();

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
    
    return true;
}

auto project_selection(Stellar::Context& context, std::shared_ptr<Stellar::Window>& window, daxa::Swapchain swapchain) -> std::string_view {
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForVulkan(window->glfw_window_ptr, true);
    daxa::ImGuiRenderer imgui_renderer =  daxa::ImGuiRenderer({
        .device = context.device,
        .format = swapchain.get_format(),
    });

    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    while(!window->should_close()) {
        glfwPollEvents();

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

        ImGui::Begin("test");

        f32 old_size = ImGui::GetFont()->Scale;
        ImGui::GetFont()->Scale *= 2;
        ImGui::PushFont(ImGui::GetFont());
        ImGui::Text("Projects");
        ImGui::GetFont()->Scale = old_size;
        ImGui::PopFont();

        if(ImGui::Button("Test")) { 
            ImGui::End();
            ImGui::Render();
            ImGui_ImplGlfw_Shutdown();
            return "test"; 
        }

        ImGui::End();
        ImGui::Render();



        daxa::ImageId swapchain_image = swapchain.acquire_next_image();
        if(swapchain_image.is_empty()) { continue; }

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

        imgui_renderer.record_commands(ImGui::GetDrawData(), cmd_list, swapchain_image, 800, 600);

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

    ImGui_ImplGlfw_Shutdown();

    return "";
}

auto main() -> i32 {
    Stellar::Context context;

    context.context = daxa::create_context({
        .enable_validation = true,
    });

    context.device = context.context.create_device({
        .enable_buffer_device_address_capture_replay = true,
        .debug_name = "my device",
    });

    context.pipeline_manager = daxa::PipelineManager({
        .device = context.device,
        .shader_compile_options = {
            .root_paths = {
                "/shaders",
                "../shaders",
                "../../shaders",
                "../../../shaders",
                "shaders",
                DAXA_SHADER_INCLUDE_DIR,
            },
            .language = daxa::ShaderLanguage::GLSL,
            .enable_debug_info = true,
        },
        .debug_name = "pipeline_manager",
    });

    std::shared_ptr<Stellar::Window> window = std::make_shared<Stellar::Window>(520, 220, "Projection selection");

    daxa::Swapchain swapchain = context.device.create_swapchain({
        .native_window = window->get_native_handle(),
        .native_window_platform = window->get_native_platform(),
        .present_mode = daxa::PresentMode::IMMEDIATE,
        .image_usage = daxa::ImageUsageFlagBits::TRANSFER_DST,
        .debug_name = "swapchain"
    });

    window->toggle_border(false);

    std::this_thread::sleep_for(50ms);

    bool loaded_sucessfully = loading_screen(context, swapchain);
    if(!loaded_sucessfully) { throw std::runtime_error("Loading project selection failed"); }

    window->toggle_border(true);
    window->set_size(800, 600);
    window->set_position(1920 + 1920 / 2 - window->width / 2, 1080 / 2 - window->height / 2);
    swapchain.resize();

    std::this_thread::sleep_for(50ms);

    swapchain.resize();
    swapchain.resize();

    std::string_view project_path = project_selection(context, window, swapchain);
    if(project_path.empty()) { throw std::runtime_error("Loading project failed"); }

    window->toggle_border(false);
    window->set_size(520, 220);
    window->set_position(1920 + 1920 / 2 - window->width / 2, 1080 / 2 - window->height / 2);
    swapchain.resize();

    std::this_thread::sleep_for(50ms);

    swapchain.resize();
    swapchain.resize();

    loaded_sucessfully = loading_screen(context, swapchain);
    if(!loaded_sucessfully) { throw std::runtime_error("Loading editor failed"); }

    Stellar::Editor editor(std::move(context), std::move(window), std::move(swapchain), project_path);
    editor.run();

    return 0;
}
