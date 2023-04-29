#pragma once

#include <core/types.hpp>

#include <daxa/daxa.hpp>
#include <daxa/utils/pipeline_manager.hpp>

namespace Stellar {
    struct Scene;

    struct DefferedRenderingSystem {
        struct DefferedRenderInfo {
            std::shared_ptr<Scene> scene;
            daxa::BufferDeviceAddress camera_buffer_address;
        };

        struct CompositionRenderInfo {
            std::shared_ptr<Scene> scene;
            daxa::SamplerId sampler;
            daxa::ImageId ssao_image;
            daxa::BufferDeviceAddress camera_buffer_address;
            f32 ambient;
        };

        DefferedRenderingSystem(daxa::Device& _device, daxa::PipelineManager pipeline_manager);
        ~DefferedRenderingSystem();

        void render_gbuffer(daxa::CommandList& cmd_list, const DefferedRenderInfo& render_info);
        void render_composition(daxa::CommandList& cmd_list, const CompositionRenderInfo& render_info);

        void resize(u32 sx, u32 sy);

        std::shared_ptr<daxa::RasterPipeline> depth_prepass_pipeline;
        std::shared_ptr<daxa::RasterPipeline> deffered_pipeline;
        std::shared_ptr<daxa::RasterPipeline> composition_pipeline;

        daxa::ImageId albedo_image;
        daxa::ImageId normal_image;
        daxa::ImageId render_image;
        daxa::ImageId depth_image;

        daxa::Device device;

        u32 size_x = 1280;
        u32 size_y = 720;
    };
}