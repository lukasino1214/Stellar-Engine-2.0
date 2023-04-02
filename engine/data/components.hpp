#pragma once

#include <core/types.hpp>
#include <core/uuid.hpp>
#include <glm/glm.hpp>
#include <entt/entt.hpp>

namespace Stellar {
    struct UUIDComponent {
        UUID uuid = UUID();
    };

    struct TagComponent {
        std::string name = "Empty Entity";
    };

    struct RelationshipComponent {
        entt::entity parent = entt::null;
        std::vector<entt::entity> children = {};
    };

    struct TransformComponent {
        glm::vec3 position = { 0.0f, 0.0f, 0.0f };
        glm::vec3 rotation = { 0.0f, 0.0f, 0.0f };
        glm::vec3 scale = { 1.0f, 1.0f, 1.0f };
        bool is_dirty = true;
    };
}