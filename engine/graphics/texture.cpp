#include "texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cstring>

namespace Stellar {
    Texture::Texture(daxa::Device _device, const std::string_view& _path, daxa::Format format) : device{_device}, path{_path} {
        i32 size_x = 0;
        i32 size_y = 0;
        i32 num_channels = 0;
        u8* data = stbi_load(path.c_str(), &size_x, &size_y, &num_channels, 4);
        if(data == nullptr) {
            throw std::runtime_error("Textures couldn't be found");
        }

        u32 mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(size_x, size_y)))) + 1;


        daxa::BufferId staging_texture_buffer = device.create_buffer({
            .memory_flags = daxa::MemoryFlagBits::HOST_ACCESS_RANDOM,
            .size = static_cast<u32>(size_x * size_y) * static_cast<u32>(4 * sizeof(u8))
        });

        image_id = device.create_image({
            .dimensions = 2,
            .format = format,
            .aspect = daxa::ImageAspectFlagBits::COLOR,
            .size = { static_cast<u32>(size_x), static_cast<u32>(size_y), 1 },
            .mip_level_count = mip_levels,
            .array_layer_count = 1,
            .sample_count = 1,
            .usage = daxa::ImageUsageFlagBits::SHADER_READ_ONLY | daxa::ImageUsageFlagBits::TRANSFER_DST | daxa::ImageUsageFlagBits::TRANSFER_SRC,
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
            .enable_anisotropy = true,
            .max_anisotropy = 16.0f,
            .enable_compare = false,
            .compare_op = daxa::CompareOp::ALWAYS,
            .min_lod = 0.0f,
            .max_lod = static_cast<f32>(mip_levels),
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
                .level_count = mip_levels,
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

        auto image_info = device.info_image(image_id);

        std::array<i32, 3> mip_size = {
            static_cast<i32>(image_info.size.x),
            static_cast<i32>(image_info.size.y),
            static_cast<i32>(image_info.size.z),
        };
        for (u32 i = 0; i < image_info.mip_level_count - 1; ++i) {
            cmd_list.pipeline_barrier_image_transition({
                .awaited_pipeline_access = daxa::AccessConsts::TRANSFER_WRITE,
                .waiting_pipeline_access = daxa::AccessConsts::BLIT_READ,
                .before_layout = daxa::ImageLayout::TRANSFER_DST_OPTIMAL,
                .after_layout = daxa::ImageLayout::TRANSFER_SRC_OPTIMAL,
                .image_slice = {
                    .image_aspect = image_info.aspect,
                    .base_mip_level = i,
                    .level_count = 1,
                    .base_array_layer = 0,
                    .layer_count = 1,
                },
                .image_id = image_id,
            });
            cmd_list.pipeline_barrier_image_transition({
                .waiting_pipeline_access = daxa::AccessConsts::BLIT_READ,
                .after_layout = daxa::ImageLayout::TRANSFER_DST_OPTIMAL,
                .image_slice = {
                    .image_aspect = image_info.aspect,
                    .base_mip_level = i + 1,
                    .level_count = 1,
                    .base_array_layer = 0,
                    .layer_count = 1,
                },
                .image_id = image_id,
            });
            std::array<i32, 3> next_mip_size = {
                std::max<i32>(1, mip_size[0] / 2),
                std::max<i32>(1, mip_size[1] / 2),
                std::max<i32>(1, mip_size[2] / 2),
            };
            cmd_list.blit_image_to_image({
                .src_image = image_id,
                .src_image_layout = daxa::ImageLayout::TRANSFER_SRC_OPTIMAL,
                .dst_image = image_id,
                .dst_image_layout = daxa::ImageLayout::TRANSFER_DST_OPTIMAL,
                .src_slice = {
                    .image_aspect = image_info.aspect,
                    .mip_level = i,
                    .base_array_layer = 0,
                    .layer_count = 1,
                },
                .src_offsets = {{{0, 0, 0}, {mip_size[0], mip_size[1], mip_size[2]}}},
                .dst_slice = {
                    .image_aspect = image_info.aspect,
                    .mip_level = i + 1,
                    .base_array_layer = 0,
                    .layer_count = 1,
                },
                .dst_offsets = {{{0, 0, 0}, {next_mip_size[0], next_mip_size[1], next_mip_size[2]}}},
                .filter = daxa::Filter::LINEAR,
            });
            mip_size = next_mip_size;
        }
        for (u32 i = 0; i < image_info.mip_level_count - 1; ++i)
        {
            cmd_list.pipeline_barrier_image_transition({
                .awaited_pipeline_access = daxa::AccessConsts::TRANSFER_READ_WRITE,
                .waiting_pipeline_access = daxa::AccessConsts::READ_WRITE,
                .before_layout = daxa::ImageLayout::TRANSFER_SRC_OPTIMAL,
                .after_layout = daxa::ImageLayout::READ_ONLY_OPTIMAL,
                .image_slice = {
                    .image_aspect = image_info.aspect,
                    .base_mip_level = i,
                    .level_count = 1,
                    .base_array_layer = 0,
                    .layer_count = 1,
                },
                .image_id = image_id,
            });
        }
        cmd_list.pipeline_barrier_image_transition({
            .awaited_pipeline_access = daxa::AccessConsts::TRANSFER_READ_WRITE,
            .waiting_pipeline_access = daxa::AccessConsts::READ_WRITE,
            .before_layout = daxa::ImageLayout::TRANSFER_DST_OPTIMAL,
            .after_layout = daxa::ImageLayout::READ_ONLY_OPTIMAL,
            .image_slice = {
                .image_aspect = image_info.aspect,
                .base_mip_level = image_info.mip_level_count - 1,
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

    auto Texture::load(daxa::Device device, const std::string_view& _path, daxa::Format format) -> std::pair<std::unique_ptr<Texture>, daxa::CommandList> {
        std::unique_ptr<Texture> texture = std::make_unique<Texture>();

        texture->device = device;
        texture->path = _path;

        i32 size_x = 0;
        i32 size_y = 0;
        i32 num_channels = 0;
        u8* data = stbi_load(texture->path.c_str(), &size_x, &size_y, &num_channels, 4);
        if(data == nullptr) {
            throw std::runtime_error("Textures couldn't be found");
        }

        u32 mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(size_x, size_y)))) + 1;

        daxa::BufferId staging_texture_buffer = device.create_buffer({
            .memory_flags = daxa::MemoryFlagBits::HOST_ACCESS_RANDOM,
            .size = static_cast<u32>(size_x * size_y) * static_cast<u32>(4 * sizeof(u8))
        });

        texture->image_id = device.create_image({
            .dimensions = 2,
            .format = format,
            .aspect = daxa::ImageAspectFlagBits::COLOR,
            .size = { static_cast<u32>(size_x), static_cast<u32>(size_y), 1 },
            .mip_level_count = mip_levels,
            .array_layer_count = 1,
            .sample_count = 1,
            .usage = daxa::ImageUsageFlagBits::SHADER_READ_ONLY | daxa::ImageUsageFlagBits::TRANSFER_DST | daxa::ImageUsageFlagBits::TRANSFER_SRC,
            .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY
        });

        texture->sampler_id = device.create_sampler({
            .magnification_filter = daxa::Filter::LINEAR,
            .minification_filter = daxa::Filter::LINEAR,
            .mipmap_filter = daxa::Filter::LINEAR,
            .address_mode_u = daxa::SamplerAddressMode::REPEAT,
            .address_mode_v = daxa::SamplerAddressMode::REPEAT,
            .address_mode_w = daxa::SamplerAddressMode::REPEAT,
            .mip_lod_bias = 0.0f,
            .enable_anisotropy = true,
            .max_anisotropy = 16.0f,
            .enable_compare = false,
            .compare_op = daxa::CompareOp::ALWAYS,
            .min_lod = 0.0f,
            .max_lod = static_cast<f32>(mip_levels),
            .enable_unnormalized_coordinates = false,
        });

        daxa::ImageId image_id = texture->image_id;

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
                .level_count = mip_levels,
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

        auto image_info = device.info_image(image_id);

        std::array<i32, 3> mip_size = {
            static_cast<i32>(image_info.size.x),
            static_cast<i32>(image_info.size.y),
            static_cast<i32>(image_info.size.z),
        };
        for (u32 i = 0; i < image_info.mip_level_count - 1; ++i) {
            cmd_list.pipeline_barrier_image_transition({
                .awaited_pipeline_access = daxa::AccessConsts::TRANSFER_WRITE,
                .waiting_pipeline_access = daxa::AccessConsts::BLIT_READ,
                .before_layout = daxa::ImageLayout::TRANSFER_DST_OPTIMAL,
                .after_layout = daxa::ImageLayout::TRANSFER_SRC_OPTIMAL,
                .image_slice = {
                    .image_aspect = image_info.aspect,
                    .base_mip_level = i,
                    .level_count = 1,
                    .base_array_layer = 0,
                    .layer_count = 1,
                },
                .image_id = image_id,
            });
            cmd_list.pipeline_barrier_image_transition({
                .waiting_pipeline_access = daxa::AccessConsts::BLIT_READ,
                .after_layout = daxa::ImageLayout::TRANSFER_DST_OPTIMAL,
                .image_slice = {
                    .image_aspect = image_info.aspect,
                    .base_mip_level = i + 1,
                    .level_count = 1,
                    .base_array_layer = 0,
                    .layer_count = 1,
                },
                .image_id = image_id,
            });
            std::array<i32, 3> next_mip_size = {
                std::max<i32>(1, mip_size[0] / 2),
                std::max<i32>(1, mip_size[1] / 2),
                std::max<i32>(1, mip_size[2] / 2),
            };
            cmd_list.blit_image_to_image({
                .src_image = image_id,
                .src_image_layout = daxa::ImageLayout::TRANSFER_SRC_OPTIMAL,
                .dst_image = image_id,
                .dst_image_layout = daxa::ImageLayout::TRANSFER_DST_OPTIMAL,
                .src_slice = {
                    .image_aspect = image_info.aspect,
                    .mip_level = i,
                    .base_array_layer = 0,
                    .layer_count = 1,
                },
                .src_offsets = {{{0, 0, 0}, {mip_size[0], mip_size[1], mip_size[2]}}},
                .dst_slice = {
                    .image_aspect = image_info.aspect,
                    .mip_level = i + 1,
                    .base_array_layer = 0,
                    .layer_count = 1,
                },
                .dst_offsets = {{{0, 0, 0}, {next_mip_size[0], next_mip_size[1], next_mip_size[2]}}},
                .filter = daxa::Filter::LINEAR,
            });
            mip_size = next_mip_size;
        }
        for (u32 i = 0; i < image_info.mip_level_count - 1; ++i)
        {
            cmd_list.pipeline_barrier_image_transition({
                .awaited_pipeline_access = daxa::AccessConsts::TRANSFER_READ_WRITE,
                .waiting_pipeline_access = daxa::AccessConsts::READ_WRITE,
                .before_layout = daxa::ImageLayout::TRANSFER_SRC_OPTIMAL,
                .after_layout = daxa::ImageLayout::READ_ONLY_OPTIMAL,
                .image_slice = {
                    .image_aspect = image_info.aspect,
                    .base_mip_level = i,
                    .level_count = 1,
                    .base_array_layer = 0,
                    .layer_count = 1,
                },
                .image_id = image_id,
            });
        }
        cmd_list.pipeline_barrier_image_transition({
            .awaited_pipeline_access = daxa::AccessConsts::TRANSFER_READ_WRITE,
            .waiting_pipeline_access = daxa::AccessConsts::READ_WRITE,
            .before_layout = daxa::ImageLayout::TRANSFER_DST_OPTIMAL,
            .after_layout = daxa::ImageLayout::READ_ONLY_OPTIMAL,
            .image_slice = {
                .image_aspect = image_info.aspect,
                .base_mip_level = image_info.mip_level_count - 1,
                .level_count = 1,
                .base_array_layer = 0,
                .layer_count = 1,
            },
            .image_id = image_id,
        });

        cmd_list.destroy_buffer_deferred(staging_texture_buffer);

        cmd_list.complete();
        stbi_image_free(data);

        return {std::move(texture), std::move(cmd_list)};
    }
}