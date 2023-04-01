#pragma once

#include <flecs.h>

#include <string_view>
#include <string>

namespace Stellar {
    struct Entity;

    struct Scene {
        Scene(const std::string_view& _name);
        ~Scene() = default;

        auto create_entity(const std::string_view& _name);
        auto create_entity_with_UUID(const std::string_view& _name, u64 uuid);

        void destroy_entity(Entity entity);

        std::string name;
        flecs::world ecs;
    };
}