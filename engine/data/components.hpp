#pragma once

#include <core/types.hpp>
#include <core/uuid.hpp>
#include <entt/entt.hpp>
#include <graphics/camera.hpp>
#include <graphics/model.hpp>

namespace Stellar {
    struct UUIDComponent {
        UUID uuid = UUID();

        void draw();
    };

    struct TagComponent {
        std::string name = "Empty Entity";

        void draw();
    };

    struct RelationshipComponent {
        entt::entity parent = entt::null;
        std::vector<entt::entity> children = {};

        void draw();
    };

    struct TransformComponent {
        glm::vec3 position = { 0.0f, 0.0f, 0.0f };
        glm::vec3 rotation{ 0.0001f, 0.0001f, 0.0001f };
        glm::vec3 scale = { 1.0f, 1.0f, 1.0f };
        glm::mat4 model_matrix{1.0f};
        glm::mat4 normal_matrix{1.0f};
        bool is_dirty = true;

        daxa::BufferId transform_buffer;

        void draw();
    };

    struct CameraComponent {
        Camera3D camera;
        bool is_dirty = true;

        void draw();
    };

    struct ModelComponent {
        std::string file_path = "";
        std::shared_ptr<Model> model;

        void draw(daxa::Device device);
    };

    struct ShadowInfo {
        glm::mat4 view{1.0f};
        glm::mat4 projection{1.0f};
        daxa::ImageId depth_image;
        daxa::ImageId shadow_image;
        daxa::ImageId temp_shadow_image;
        bool has_to_resize = false;
        glm::ivec2 image_size = {1024, 1024};
        f32 clip_space = 128.0f;
    };

    struct DirectionalLightComponent {
        glm::vec3 color = { 1.0f, 1.0f, 1.0f };
        f32 intensity = 1.0f;
        bool is_dirty = true;
        ShadowInfo shadow_info;

        void draw();
    };

    struct PointLightComponent {
        glm::vec3 color = { 1.0f, 1.0f, 1.0f };
        f32 intensity = 32.0f;
        bool is_dirty = true;

        void draw();
    };

    struct SpotLightComponent {
        glm::vec3 color = { 1.0f, 1.0f, 1.0f };
        f32 intensity = 4.0f;
        f32 cut_off = 20.0f;
        f32 outer_cut_off = 30.0f;
        bool is_dirty = true;
        ShadowInfo shadow_info;

        void draw();
    };
}