#pragma once

#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>

#include <data/components.hpp>
#include <core/types.hpp>

#include <string_view>

namespace Stellar {
    struct Entity;

    struct Scene {
        Scene(const std::string_view& _name);
        ~Scene() = default;

        auto create_entity(const std::string_view& _name) -> Entity;
        auto create_entity_with_UUID(const std::string_view& _name, UUID _uuid) -> Entity;

        void destroy_entity(Entity entity);

        void serialize(const std::string_view& path);
        void deserialize(const std::string_view& path);

        void reset();

        std::string name;
        std::unique_ptr<entt::registry> registry;
    };
}