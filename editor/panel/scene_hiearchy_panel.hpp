#pragma once

#include <core/types.hpp>
#include <data/scene.hpp>
#include <data/entity.hpp>

namespace Stellar {
    struct SceneHiearchyPanel {
        explicit SceneHiearchyPanel(const std::shared_ptr<Scene>& _scene);
        ~SceneHiearchyPanel() = default;

        void draw();

        void set_context(const std::shared_ptr<Scene>& _scene);

        void tree(Entity& entity, const RelationshipComponent &relationship_component, const u32 iteration);

        Entity selected_entity;
        std::shared_ptr<Scene> scene;
    };
}