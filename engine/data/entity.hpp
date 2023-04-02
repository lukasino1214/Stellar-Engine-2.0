#pragma once

#include <data/components.hpp>
#include <entt/entt.hpp>
#include <data/scene.hpp>

namespace Stellar {
    struct Entity {
        Entity() = default;
		Entity(entt::entity _handle, Scene* _scene);
		Entity(const Entity& other) = default;
        ~Entity() = default;

        operator bool() const { return handle != entt::null; }
		operator entt::entity() const { return handle; }
		operator u32() const { return static_cast<u32>(handle); }

        auto get_name() -> std::string_view;
        auto get_uuid() -> UUID;

        template<typename T, typename... Args>
		auto add_component(Args&&... args) -> T& {
            T& component = scene->registry->emplace<T>(handle, std::forward<Args>(args)...);
			return component;
		}

        template<typename T>
        auto get_component() -> T& {
            return scene->registry->get<T>(handle);
        }

        template<typename T>
        auto has_component() -> bool {
            return scene->registry->all_of<T>(handle);
        }

        template<typename T>
        void remove_component() {
            scene->registry->remove<T>(handle);
        }

        entt::entity handle{ entt::null };
		Scene* scene = nullptr;
    };
}