#pragma once

#include <daxa/daxa.hpp>
using namespace daxa::types;

namespace Stellar {
    struct Texture {
        Texture() = default;
        Texture(daxa::Device _device, const std::string_view& _path, daxa::Format format);
        ~Texture();

        static auto load(daxa::Device device, const std::string_view& _path, daxa::Format format) -> std::pair<std::unique_ptr<Texture>, daxa::CommandList>;

        daxa::Device device;
        daxa::ImageId image_id;
        daxa::SamplerId sampler_id;
        std::string path;
    };
}