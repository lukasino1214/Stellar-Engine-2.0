#include "data/components.hpp"
#include "data/scene.hpp"
#include <data/entity.hpp>
#include <string_view>

namespace Stellar {
    Entity::Entity(entt::entity _handle, Scene* _scene) : handle{_handle}, scene{_scene} {}

    auto Entity::get_name() -> std::string_view {
        return get_component<TagComponent>().name;
    }

    auto Entity::get_uuid() -> UUID {
        return get_component<UUIDComponent>().uuid;
    }
}