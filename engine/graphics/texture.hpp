#pragma once

#include <daxa/daxa.hpp>
using namespace daxa::types;

namespace Stellar {
    struct Texture {
        Texture(daxa::Device _device, const std::string_view& _path, daxa::Format format);
        ~Texture();

        daxa::Device device;
        daxa::ImageId image_id;
        daxa::SamplerId sampler_id;
        std::string path;
    };
}