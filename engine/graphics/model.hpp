#pragma once
#include <core/types.hpp>

#include <daxa/daxa.hpp>

#include <graphics/texture.hpp>
#include "../../shaders/shared.inl"
#include <physics/aabb.hpp>

namespace Stellar {
    struct Primitive {
        u32 first_index;
        u32 first_vertex;
        u32 index_count;
        u32 vertex_count;
        u32 material_index;
        AABB aabb;
    };

    struct Model {
        Model(daxa::Device _device, const std::string_view& file_path);
        ~Model();

        void draw(daxa::CommandList& cmd_list, DepthPrepassPush& draw_push);
        void draw(daxa::CommandList& cmd_list, DrawPush& draw_push);
        void draw(daxa::CommandList& cmd_list, ShadowPush& draw_push);
        void draw(daxa::CommandList& cmd_list, SkyPush& draw_push);

        daxa::Device device;
        daxa::BufferId face_buffer = {};
        daxa::BufferId index_buffer = {};
        daxa::BufferId material_info_buffer = {};
        
        std::vector<std::unique_ptr<Texture>> textures = {};
        std::vector<MaterialInfo> materials = {};
        std::vector<Primitive> primitives = {};
    };
}