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
        glm::vec3 rotation = { 0.0f, 0.0f, 0.0f };
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
        std::shared_ptr<Model> model;

        void draw();
    };

    struct DirectionalLightComponent {
        glm::vec3 color = { 1.0f, 1.0f, 1.0f };
        f32 intensity = 4.0f;
        bool is_dirty = true;

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
        f32 intensity = 32.0f;
        f32 cut_off = 20.0f;
        f32 outer_cut_off = 30.0f;
        bool is_dirty = true;

        void draw();
    };
}