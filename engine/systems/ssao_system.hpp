#pragma once

#include <core/types.hpp>

#include <daxa/daxa.hpp>
#include <daxa/utils/pipeline_manager.hpp>

namespace Stellar {
    struct SSAOSystem {
        struct RenderInfo {
            daxa::ImageId depth_image;
            daxa::ImageId normal_image;
            daxa::SamplerId sampler;
            daxa::BufferDeviceAddress camera_buffer_address;
        };

        SSAOSystem(daxa::Device& _device, daxa::PipelineManager pipeline_manager);
        ~SSAOSystem();

        void render(daxa::CommandList& cmd_list, const RenderInfo& render_info);

        void resize(u32 sx, u32 sy);

        std::shared_ptr<daxa::RasterPipeline> ssao_generation_pipeline;
        std::shared_ptr<daxa::RasterPipeline> ssao_blur_pipeline;

        daxa::ImageId ssao_image;
        daxa::ImageId ssao_blur_image;

        daxa::Device device;

        u32 size_x = 1280;
        u32 size_y = 720;
    };
}