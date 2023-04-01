#pragma once

#include <daxa/daxa.hpp>
using namespace daxa::types;

namespace Stellar {
    struct Texture {
        Texture(daxa::Device _device, const std::string& path);
        ~Texture();

        daxa::Device device;
        daxa::ImageId image_id;
        daxa::SamplerId sampler_id;
    };
}