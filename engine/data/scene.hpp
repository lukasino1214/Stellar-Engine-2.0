#pragma once

#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>

#include <data/components.hpp>
#include <core/types.hpp>

#include <functional>

namespace Stellar {
    struct Entity;

    struct Scene {
        explicit Scene(const std::string_view& _name, daxa::Device _device);
        ~Scene();

        auto create_entity(const std::string_view& _name) -> Entity;
        auto create_entity_with_UUID(const std::string_view& _name, UUID _uuid) -> Entity;

        void destroy_entity(const Entity& entity);

        void iterate(std::function<void(Entity)> fn);

        void serialize(const std::string_view& path);
        void deserialize(const std::string_view& path);

        void reset();

        void update();

        std::string name;
        std::unique_ptr<entt::registry> registry;
        daxa::Device device;

        daxa::BufferId light_buffer;
    };
}