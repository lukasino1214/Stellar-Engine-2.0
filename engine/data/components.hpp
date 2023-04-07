#pragma once

#include <core/types.hpp>
#include <core/uuid.hpp>
#include <entt/entt.hpp>
#include <graphics/camera.hpp>

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

        void draw();
    };

    struct CameraComponent {
        Camera3D camera;
        bool is_dirty = true;

        void draw();
    };
}