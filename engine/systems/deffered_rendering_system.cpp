#include "deffered_rendering_system.hpp"

#include "../../shaders/shared.inl"

#include <data/entity.hpp>
#include <data/scene.hpp>
#include <data/components.hpp>
#include <graphics/model.hpp>

namespace Stellar {
    DefferedRenderingSystem::DefferedRenderingSystem(daxa::Device& _device, daxa::PipelineManager pipeline_manager) : device{_device} {
        depth_prepass_pipeline = pipeline_manager.add_raster_pipeline({
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

        deffered_pipeline = pipeline_manager.add_raster_pipeline({
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

        composition_pipeline = pipeline_manager.add_raster_pipeline({
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
        
        albedo_image = device.create_image({
            .format = daxa::Format::R8G8B8A8_SRGB,
            .aspect = daxa::ImageAspectFlagBits::COLOR,
            .size = { size_x, size_y, 1 },
            .usage = daxa::ImageUsageFlagBits::COLOR_ATTACHMENT | daxa::ImageUsageFlagBits::SHADER_READ_ONLY | daxa::ImageUsageFlagBits::TRANSFER_DST,
            .debug_name = "albedo_image"
        });

        normal_image = device.create_image({
            .format = daxa::Format::R16G16B16A16_SFLOAT,
            .aspect = daxa::ImageAspectFlagBits::COLOR,
            .size = { size_x, size_y, 1 },
            .usage = daxa::ImageUsageFlagBits::COLOR_ATTACHMENT | daxa::ImageUsageFlagBits::SHADER_READ_ONLY | daxa::ImageUsageFlagBits::TRANSFER_DST,
            .debug_name = "normal_image"
        });

        render_image = device.create_image({
            .format = daxa::Format::R8G8B8A8_SRGB,
            .aspect = daxa::ImageAspectFlagBits::COLOR,
            .size = { size_x, size_y, 1 },
            .usage = daxa::ImageUsageFlagBits::COLOR_ATTACHMENT | daxa::ImageUsageFlagBits::SHADER_READ_ONLY | daxa::ImageUsageFlagBits::TRANSFER_DST,
            .debug_name = "render_image"
        });

        depth_image = device.create_image({
            .format = daxa::Format::D32_SFLOAT,
            .aspect = daxa::ImageAspectFlagBits::DEPTH,
            .size = { size_x, size_y, 1 },
            .usage = daxa::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT,
            .debug_name = "depth_image"
        });
    }

    DefferedRenderingSystem::~DefferedRenderingSystem() {
        device.destroy_image(albedo_image);
        device.destroy_image(normal_image);
        device.destroy_image(render_image);
        device.destroy_image(depth_image);
    }

    void DefferedRenderingSystem::render_gbuffer(daxa::CommandList &cmd_list, const DefferedRenderInfo& render_info) {
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

        cmd_list.begin_renderpass({
            .depth_attachment = {{
                .image_view = depth_image.default_view(),
                .load_op = daxa::AttachmentLoadOp::CLEAR,
                .clear_value = daxa::DepthValue{1.0f, 0},
            }},
            .render_area = {.x = 0, .y = 0, .width = size_x, .height = size_y},
        });
 
        cmd_list.set_pipeline(*depth_prepass_pipeline);

        render_info.scene->iterate([&](Entity entity){
            if(entity.has_component<ModelComponent>()) {
                auto& tc = entity.get_component<TransformComponent>();
                auto& mc = entity.get_component<ModelComponent>();
                if(!mc.model) { return; }

                DepthPrepassPush draw_push;
                draw_push.camera_info = render_info.camera_buffer_address;
                draw_push.transform_buffer = device.get_device_address(tc.transform_buffer);
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
            .render_area = {.x = 0, .y = 0, .width = size_x, .height = size_y},
        });
 
        cmd_list.set_pipeline(*deffered_pipeline);

        render_info.scene->iterate([&](Entity entity){
            if(entity.has_component<ModelComponent>()) {
                auto& tc = entity.get_component<TransformComponent>();
                auto& mc = entity.get_component<ModelComponent>();
                if(!mc.model) { return; }

                DrawPush draw_push;
                draw_push.camera_info = render_info.camera_buffer_address;
                draw_push.transform_buffer = device.get_device_address(tc.transform_buffer);
                draw_push.light_buffer = device.get_device_address(render_info.scene->light_buffer);
                mc.model->draw(cmd_list, draw_push);
            }
        });

        cmd_list.end_renderpass();
    }

    void DefferedRenderingSystem::render_composition(daxa::CommandList &cmd_list, const CompositionRenderInfo &render_info) {
        cmd_list.begin_renderpass({
            .color_attachments = {
                {
                    .image_view = this->render_image.default_view(),
                    .load_op = daxa::AttachmentLoadOp::CLEAR,
                    .clear_value = std::array<f32, 4>{0.05f, 0.05f, 0.05f, 1.0f},
                },
            },
            .render_area = {.x = 0, .y = 0, .width = size_x, .height = size_y },
        });

        cmd_list.set_pipeline(*composition_pipeline);
        cmd_list.push_constant(CompositionPush {
            .albedo = { .texture_id = albedo_image.default_view(), .sampler_id = render_info.sampler },
            .normal = { .texture_id = normal_image.default_view(), .sampler_id = render_info.sampler },
            .depth = { .texture_id = depth_image.default_view(), .sampler_id = render_info.sampler },
            .ssao = { .texture_id = render_info.ssao_image.default_view(), .sampler_id = render_info.sampler },
            .light_buffer = device.get_device_address(render_info.scene->light_buffer),
            .camera_info = render_info.camera_buffer_address,
            .ambient = render_info.ambient
        });

        cmd_list.draw({ .vertex_count = 3 });
        cmd_list.end_renderpass();
    }

    void DefferedRenderingSystem::resize(u32 sx, u32 sy) {
        size_x = sx;
        size_y = sy;

        device.destroy_image(albedo_image);
        albedo_image = device.create_image({
            .format = daxa::Format::R8G8B8A8_SRGB,
            .aspect = daxa::ImageAspectFlagBits::COLOR,
            .size = { size_x, size_y, 1 },
            .usage = daxa::ImageUsageFlagBits::COLOR_ATTACHMENT | daxa::ImageUsageFlagBits::SHADER_READ_ONLY | daxa::ImageUsageFlagBits::TRANSFER_DST,
            .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
            .debug_name = "albedo_image"
        });

        device.destroy_image(normal_image);
        normal_image = device.create_image({
            .format = daxa::Format::R16G16B16A16_SFLOAT,
            .aspect = daxa::ImageAspectFlagBits::COLOR,
            .size = { size_x, size_y, 1 },
            .usage = daxa::ImageUsageFlagBits::COLOR_ATTACHMENT | daxa::ImageUsageFlagBits::SHADER_READ_ONLY | daxa::ImageUsageFlagBits::TRANSFER_DST,
            .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
            .debug_name = "normal_image"
        });

        device.destroy_image(render_image);
        render_image = device.create_image({
            .format = daxa::Format::R8G8B8A8_SRGB,
            .aspect = daxa::ImageAspectFlagBits::COLOR,
            .size = { size_x, size_y, 1 },
            .usage = daxa::ImageUsageFlagBits::COLOR_ATTACHMENT | daxa::ImageUsageFlagBits::SHADER_READ_ONLY | daxa::ImageUsageFlagBits::TRANSFER_DST,
            .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
            .debug_name = "render_image"
        });

        device.destroy_image(depth_image);
        depth_image = device.create_image({
            .format = daxa::Format::D32_SFLOAT,
            .aspect = daxa::ImageAspectFlagBits::DEPTH,
            .size = { size_x, size_y, 1 },
            .usage = daxa::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT | daxa::ImageUsageFlagBits::SHADER_READ_ONLY,
            .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
            .debug_name = "depth_image"
        });
    }
}