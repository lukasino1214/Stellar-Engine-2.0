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

// #include <ctype.h>

// #include "PxPhysicsAPI.h"

// using namespace physx;

// PxDefaultAllocator		gAllocator;
// PxDefaultErrorCallback	gErrorCallback;

// PxFoundation*			gFoundation = NULL;
// PxPhysics*				gPhysics	= NULL;

// PxDefaultCpuDispatcher*	gDispatcher = NULL;
// PxScene*				gScene		= NULL;

// PxMaterial*				gMaterial	= NULL;

// PxPvd*                  gPvd        = NULL;

// PxReal stackZ = 10.0f;

// PxRigidDynamic* createDynamic(const PxTransform& t, const PxGeometry& geometry, const PxVec3& velocity=PxVec3(0))
// {
// 	PxRigidDynamic* dynamic = PxCreateDynamic(*gPhysics, t, geometry, *gMaterial, 10.0f);
// 	dynamic->setAngularDamping(0.5f);
// 	dynamic->setLinearVelocity(velocity);
// 	gScene->addActor(*dynamic);
// 	return dynamic;
// }


// void createStack(const PxTransform& t, PxU32 size, PxReal halfExtent)
// {
// 	PxShape* shape = gPhysics->createShape(PxBoxGeometry(halfExtent, halfExtent, halfExtent), *gMaterial);
// 	for(PxU32 i=0; i<size;i++)
// 	{
// 		for(PxU32 j=0;j<size-i;j++)
// 		{
// 			PxTransform localTm(PxVec3(PxReal(j*2) - PxReal(size-i), PxReal(i*2+1), 0) * halfExtent);
// 			PxRigidDynamic* body = gPhysics->createRigidDynamic(t.transform(localTm));
// 			body->attachShape(*shape);
// 			PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
// 			gScene->addActor(*body);
// 		}
// 	}
// 	shape->release();
// }

// void initPhysics(bool interactive)
// {
// 	gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);

// 	gPvd = PxCreatePvd(*gFoundation);
// 	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
// 	gPvd->connect(*transport,PxPvdInstrumentationFlag::eALL);

// 	gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(),true,gPvd);

// 	PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
// 	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
// 	gDispatcher = PxDefaultCpuDispatcherCreate(2);
// 	sceneDesc.cpuDispatcher	= gDispatcher;
// 	sceneDesc.filterShader	= PxDefaultSimulationFilterShader;
// 	gScene = gPhysics->createScene(sceneDesc);

// 	PxPvdSceneClient* pvdClient = gScene->getScenePvdClient();
// 	if(pvdClient)
// 	{
// 		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
// 		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
// 		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
// 	}
// 	gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.6f);

// 	PxRigidStatic* groundPlane = PxCreatePlane(*gPhysics, PxPlane(0,1,0,0), *gMaterial);
// 	gScene->addActor(*groundPlane);

// 	for(PxU32 i=0;i<5;i++)
// 		createStack(PxTransform(PxVec3(0,0,stackZ-=10.0f)), 10, 2.0f);

// 	if(!interactive)
// 		createDynamic(PxTransform(PxVec3(0,40,100)), PxSphereGeometry(10), PxVec3(0,-50,-100));
// }

// void stepPhysics(bool /*interactive*/)
// {
// 	gScene->simulate(1.0f/60.0f);
// 	gScene->fetchResults(true);
//     //std::cout << negr->getGlobalPose().p.x << " " << negr->getGlobalPose().p.y << " " << negr->getGlobalPose().p.y << std::endl;
// }
	
// void cleanupPhysics(bool /*interactive*/)
// {
// 	gScene->release();
// 	gDispatcher->release();
// 	gPhysics->release();
// 	if(gPvd)
// 	{
// 		PxPvdTransport* transport = gPvd->getTransport();
// 		gPvd->release();	gPvd = NULL;
// 		transport->release();
// 	}
// 	gFoundation->release();
	
// 	printf("SnippetHelloWorld done.\n");
// }

// void keyPress(unsigned char key, const PxTransform& camera)
// {
// 	switch(toupper(key))
// 	{
// 	case 'B':	createStack(PxTransform(PxVec3(0,0,stackZ-=10.0f)), 10, 2.0f);						break;
// 	case ' ':	createDynamic(camera, PxSphereGeometry(3.0f), camera.rotate(PxVec3(0,0,-1))*200);	break;
// 	}
// }

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


    /*static const PxU32 frameCount = 10000;
	initPhysics(false);
    PxShape* shape = gPhysics->createShape(PxBoxGeometry(1, 1, 1), *gMaterial);
    PxTransform localTm(PxVec3(0, 100, 0));
    PxRigidDynamic* body = gPhysics->createRigidDynamic(localTm);
    body->attachShape(*shape);
    PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
    gScene->addActor(*body);
	for(PxU32 i=0; i<frameCount; i++) {
		stepPhysics(false);
        std::cout << body->getGlobalPose().p.x << " " << body->getGlobalPose().p.y << " " << body->getGlobalPose().p.z << std::endl;
    }
	cleanupPhysics(false);

    return 0;*/

    Stellar::Editor editor(std::move(context), std::move(window), std::move(swapchain), project_path);
    editor.run();

    return 0;
}
