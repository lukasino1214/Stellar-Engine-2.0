#include "texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cstring>

namespace Stellar {
    Texture::Texture(daxa::Device _device, const std::string_view& _path) : device{_device}, path{_path} {
        i32 size_x = 0;
        i32 size_y = 0;
        i32 num_channels = 0;
        u8* data = stbi_load(path.c_str(), &size_x, &size_y, &num_channels, 4);
        if(data == nullptr) {
            throw std::runtime_error("Textures couldn't be found");
        }

        daxa::BufferId staging_texture_buffer = device.create_buffer({
            .memory_flags = daxa::MemoryFlagBits::HOST_ACCESS_RANDOM,
            .size = static_cast<u32>(size_x * size_y) * static_cast<u32>(4 * sizeof(u8))
        });

        image_id = device.create_image({
            .dimensions = 2,
            .format = daxa::Format::R8G8B8A8_SRGB,
            .aspect = daxa::ImageAspectFlagBits::COLOR,
            .size = { static_cast<u32>(size_x), static_cast<u32>(size_y), 1 },
            .mip_level_count = 1,
            .array_layer_count = 1,
            .sample_count = 1,
            .usage = daxa::ImageUsageFlagBits::SHADER_READ_ONLY | daxa::ImageUsageFlagBits::TRANSFER_DST,
            .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY
        });

        sampler_id = device.create_sampler({
            .magnification_filter = daxa::Filter::LINEAR,
            .minification_filter = daxa::Filter::LINEAR,
            .mipmap_filter = daxa::Filter::LINEAR,
            .address_mode_u = daxa::SamplerAddressMode::REPEAT,
            .address_mode_v = daxa::SamplerAddressMode::REPEAT,
            .address_mode_w = daxa::SamplerAddressMode::REPEAT,
            .mip_lod_bias = 0.0f,
            .enable_anisotropy = false,
            .max_anisotropy = 0.0f,
            .enable_compare = false,
            .compare_op = daxa::CompareOp::ALWAYS,
            .min_lod = 0.0f,
            .max_lod = static_cast<f32>(1),
            .enable_unnormalized_coordinates = false,
        });

        auto* staging_texture_buffer_ptr = device.get_host_address_as<u8>(staging_texture_buffer);
        std::memcpy(staging_texture_buffer_ptr, data, static_cast<u32>(size_x * size_y) * static_cast<u32>(4 * sizeof(u8)));

        daxa::CommandList cmd_list = device.create_command_list({ .debug_name = "upload cmd list" });

        cmd_list.pipeline_barrier_image_transition({
            .awaited_pipeline_access = daxa::AccessConsts::TRANSFER_READ_WRITE,
            .waiting_pipeline_access = daxa::AccessConsts::READ_WRITE,
            .before_layout = daxa::ImageLayout::UNDEFINED,
            .after_layout = daxa::ImageLayout::TRANSFER_DST_OPTIMAL,
            .image_slice = {
                .image_aspect = daxa::ImageAspectFlagBits::COLOR,
                .base_mip_level = 0,
                .level_count = 1,
                .base_array_layer = 0,
                .layer_count = 1,
            },
            .image_id = image_id,
        });

        cmd_list.copy_buffer_to_image({
            .buffer = staging_texture_buffer,
            .buffer_offset = 0,
            .image = image_id,
            .image_layout = daxa::ImageLayout::TRANSFER_DST_OPTIMAL,
            .image_slice = {
                .image_aspect = daxa::ImageAspectFlagBits::COLOR,
                .mip_level = 0,
                .base_array_layer = 0,
                .layer_count = 1,
            },
            .image_offset = { 0, 0, 0 },
            .image_extent = { static_cast<u32>(size_x), static_cast<u32>(size_y), 1 }
        });

        cmd_list.pipeline_barrier_image_transition({
            .awaited_pipeline_access = daxa::AccessConsts::TRANSFER_READ_WRITE,
            .waiting_pipeline_access = daxa::AccessConsts::READ_WRITE,
            .before_layout = daxa::ImageLayout::TRANSFER_DST_OPTIMAL,
            .after_layout = daxa::ImageLayout::READ_ONLY_OPTIMAL,
            .image_slice = {
                .image_aspect = daxa::ImageAspectFlagBits::COLOR,
                .base_mip_level = 0,
                .level_count = 1,
                .base_array_layer = 0,
                .layer_count = 1,
            },
            .image_id = image_id,
        });

        cmd_list.complete();

        device.submit_commands({
            .command_lists = {std::move(cmd_list)},
        });
        device.wait_idle();
        device.destroy_buffer(staging_texture_buffer);

        stbi_image_free(data);
    }

    Texture::~Texture() {
        device.destroy_image(image_id);
        device.destroy_sampler(sampler_id);
    }
}