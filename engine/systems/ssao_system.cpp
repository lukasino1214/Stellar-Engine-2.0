#include "ssao_system.hpp"

#include "../../shaders/shared.inl"

namespace Stellar {
    SSAOSystem::SSAOSystem(daxa::Device& _device, daxa::PipelineManager pipeline_manager) : device{_device} {
        this->ssao_generation_pipeline = pipeline_manager.add_raster_pipeline({
            .vertex_shader_info = { .source = daxa::ShaderFile{"ssao_generation.glsl"}, .compile_options = { .defines = { daxa::ShaderDefine{"DRAW_VERT"}}}},
            .fragment_shader_info = { .source = daxa::ShaderFile{"ssao_generation.glsl"}, .compile_options = { .defines = { daxa::ShaderDefine{"DRAW_FRAG"}}}},
            .color_attachments = { { .format = daxa::Format::R8_UNORM, } },
            .raster = { .polygon_mode = daxa::PolygonMode::FILL, },
            .push_constant_size = sizeof(SSAOGenerationPush),
            .debug_name = "ssao_generation_code",
        }).value();

        this->ssao_blur_pipeline = pipeline_manager.add_raster_pipeline({
            .vertex_shader_info = { .source = daxa::ShaderFile{"ssao_blur.glsl"}, .compile_options = { .defines = { daxa::ShaderDefine{"DRAW_VERT"}}}},
            .fragment_shader_info = { .source = daxa::ShaderFile{"ssao_blur.glsl"}, .compile_options = { .defines = { daxa::ShaderDefine{"DRAW_FRAG"}}}},
            .color_attachments = { { .format = daxa::Format::R8_UNORM, } },
            .raster = { .polygon_mode = daxa::PolygonMode::FILL, },
            .push_constant_size = sizeof(SSAOBlurPush),
            .debug_name = "ssao_blur_pipeline",
        }).value();

        this->ssao_image = device.create_image({
            .format = daxa::Format::R8_UNORM,
            .aspect = daxa::ImageAspectFlagBits::COLOR,
            .size = { size_x / 2, size_y / 2, 1 },
            .usage = daxa::ImageUsageFlagBits::COLOR_ATTACHMENT | daxa::ImageUsageFlagBits::SHADER_READ_ONLY,
            .debug_name = "ssao_image"
        });

        this->ssao_blur_image = device.create_image({
            .format = daxa::Format::R8_UNORM,
            .aspect = daxa::ImageAspectFlagBits::COLOR,
            .size = { size_x / 2, size_y / 2, 1 },
            .usage = daxa::ImageUsageFlagBits::COLOR_ATTACHMENT | daxa::ImageUsageFlagBits::SHADER_READ_ONLY,
            .debug_name = "ssao_blur_image"
        });
    }

    SSAOSystem::~SSAOSystem() {
        device.destroy_image(ssao_image);
        device.destroy_image(ssao_blur_image);
    }

    void SSAOSystem::render(daxa::CommandList &cmd_list, const RenderInfo& render_info) {
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
            .color_attachments = {{
                    .image_view = this->ssao_image.default_view(),
                    .load_op = daxa::AttachmentLoadOp::CLEAR,
                    .clear_value = std::array<f32, 4>{1.0f, 0.0f, 0.0f, 1.0f},
            }},
            .render_area = {.x = 0, .y = 0, .width = size_x / 2, .height = size_y / 2},
        });
        cmd_list.set_pipeline(*ssao_generation_pipeline);
        cmd_list.push_constant(SSAOGenerationPush {
            .normal = { .texture_id = render_info.normal_image.default_view(), .sampler_id = render_info.sampler },
            .depth = { .texture_id = render_info.depth_image.default_view(), .sampler_id = render_info.sampler },
            .camera_info = render_info.camera_buffer_address
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
            .render_area = {.x = 0, .y = 0, .width = size_x / 2, .height = size_y / 2},
        });
        cmd_list.set_pipeline(*ssao_blur_pipeline);
        cmd_list.push_constant(SSAOBlurPush {
            .ssao = { .texture_id = ssao_image.default_view(), .sampler_id = render_info.sampler },
        });
        cmd_list.draw({ .vertex_count = 3 });
        cmd_list.end_renderpass();
    }

    void SSAOSystem::resize(u32 sx, u32 sy) {
        size_x = sx;
        size_y = sy;

        device.destroy_image(ssao_image);
        this->ssao_image = device.create_image({
            .format = daxa::Format::R8_UNORM,
            .aspect = daxa::ImageAspectFlagBits::COLOR,
            .size = { size_x / 2, size_y / 2, 1 },
            .usage = daxa::ImageUsageFlagBits::COLOR_ATTACHMENT | daxa::ImageUsageFlagBits::SHADER_READ_ONLY,
            .debug_name = "ssao_image"
        });

        device.destroy_image(ssao_blur_image);
        this->ssao_blur_image = device.create_image({
            .format = daxa::Format::R8_UNORM,
            .aspect = daxa::ImageAspectFlagBits::COLOR,
            .size = { size_x / 2, size_y / 2, 1 },
            .usage = daxa::ImageUsageFlagBits::COLOR_ATTACHMENT | daxa::ImageUsageFlagBits::SHADER_READ_ONLY,
            .debug_name = "ssao_blur_image"
        });
    }
}