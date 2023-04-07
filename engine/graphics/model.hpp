#pragma once
#include <core/types.hpp>

#include <daxa/daxa.hpp>

#include <graphics/texture.hpp>

#include "../../shaders/shared.inl"

namespace Stellar {
    struct Material {
        u32 albedo_texture_index;
    };

    struct Primitive {
        u32 first_index;
        u32 first_vertex;
        u32 index_count;
        u32 vertex_count;
        u32 material_index;
    };

    struct Model {
        Model(daxa::Device _device, const std::string_view& file_path);
        ~Model();

        void draw(daxa::CommandList& cmd_list, DepthPrepassPush& draw_push);
        void draw(daxa::CommandList& cmd_list, DrawPush& draw_push);

        daxa::Device device;
        daxa::BufferId face_buffer = {};
        daxa::BufferId index_buffer = {};
        
        std::vector<std::unique_ptr<Texture>> textures = {};
        std::vector<Material> materials = {};
        std::vector<Primitive> primitives = {};
    };
}