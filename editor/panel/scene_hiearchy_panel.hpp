#pragma once

#include <core/types.hpp>
#include <data/scene.hpp>
#include <data/entity.hpp>
#include <physics/physics.hpp>

namespace Stellar {
    struct SceneHiearchyPanel {
        explicit SceneHiearchyPanel(const std::shared_ptr<Scene>& _scene, const std::shared_ptr<Physics>& _physics);
        ~SceneHiearchyPanel() = default;

        void draw();

        void set_context(const std::shared_ptr<Scene>& _scene);

        void tree(Entity& entity, const RelationshipComponent &relationship_component, const u32 iteration);

        template<typename T>
        void draw_component(Entity entity, const std::string_view& component_name);

        Entity selected_entity;
        std::shared_ptr<Scene> scene;
        std::shared_ptr<Physics> physics;
    };
}