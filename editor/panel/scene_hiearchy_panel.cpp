#include "scene_hiearchy_panel.hpp"

#include <data/components.hpp>
#include <data/scene.hpp>

#include <imgui.h>
#include <imgui_internal.h>

#include <utils/gui.hpp>

#include <iostream>

namespace Stellar {
    template<typename T>
    void SceneHiearchyPanel::draw_component(Entity entity, const std::string_view& component_name) {
        const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
        if (entity.has_component<T>()) {
            auto& component = entity.get_component<T>();
            ImVec2 content_region_available = ImGui::GetContentRegionAvail();

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });

            f32 line_height = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;

            bool open = ImGui::TreeNodeEx(reinterpret_cast<void*>(typeid(T).hash_code() << static_cast<uint32_t>(entity.handle)), treeNodeFlags, "%s", component_name.data());
            ImGui::PopStyleVar();

            ImGui::SameLine(content_region_available.x - line_height * 0.5f);
            if (ImGui::Button("+", ImVec2{ line_height, line_height })) {
                ImGui::OpenPopup("ComponentSettings");
            }

            bool remove_component = false;
            if (ImGui::BeginPopup("ComponentSettings")) {
                if (ImGui::MenuItem("Remove component")) {
                    remove_component = true;
                }

                ImGui::EndPopup();
            }

            if (open) {
                if constexpr(std::is_same_v<T, ModelComponent>) {
                    component.draw(scene->device);
                } else if constexpr(std::is_same_v<T, RigidBodyComponent>) {
                    component.draw(physics, entity);
                } else if constexpr(std::is_same_v<T, TransformComponent>) {
                    component.draw(entity);
                } else {
                    component.draw();
                }
                GUI::indent(0.0f, GImGui->Style.ItemSpacing.y);
                ImGui::TreePop();
            }

            if (remove_component) {
//                World::RemoveComponent<T>(entity);
            }
        }
    }

    template<typename T>
    inline void add_new_component(Entity entity, const std::string_view& component_name) {
        if(!entity.has_component<T>()) {
            if (ImGui::MenuItem(component_name.data())) {
                entity.add_component<T>();
            
                if constexpr(std::is_same_v<T, ModelComponent>) {
                    entity.try_add_component<TransformComponent>();
                } else if constexpr(std::is_same_v<T, DirectionalLightComponent>) {
                    entity.try_add_component<TransformComponent>();
                } else if constexpr(std::is_same_v<T, PointLightComponent>) {
                    entity.try_add_component<TransformComponent>();
                } else if constexpr(std::is_same_v<T, SpotLightComponent>) {
                    entity.try_add_component<TransformComponent>();
                } else if constexpr(std::is_same_v<T, RigidBodyComponent>) {
                    entity.try_add_component<TransformComponent>();
                }
            }
        }
    }

    SceneHiearchyPanel::SceneHiearchyPanel(const std::shared_ptr<Scene>& _scene, const std::shared_ptr<Physics>& _physics) : scene{_scene}, physics{_physics} {}

    void SceneHiearchyPanel::tree(Entity& entity, const RelationshipComponent &relationship_component, const u32 iteration) {
        ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanAvailWidth;

        bool selected = false;
        if (selected_entity) {
            if(selected_entity.get_component<UUIDComponent>().uuid.uuid == entity.get_component<UUIDComponent>().uuid.uuid) {
                selected = true;
                treeNodeFlags |= ImGuiTreeNodeFlags_Selected;
                ImGui::PushStyleColor(ImGuiCol_Header, { 0.18823529411f, 0.18823529411f, 0.18823529411f, 1.0f });
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, { 0.18823529411f, 0.18823529411f, 0.18823529411f, 1.0f });
            }
        }

        bool opened = ImGui::TreeNodeEx(reinterpret_cast<void*>(entity.get_component<UUIDComponent>().uuid.uuid), treeNodeFlags, "%s", entity.get_component<TagComponent>().name.c_str());

        if (ImGui::IsItemClicked(ImGuiPopupFlags_MouseButtonLeft) || ImGui::IsItemClicked(ImGuiPopupFlags_MouseButtonRight)) {
            selected_entity = entity;
        }

        bool entity_deleted = false;
		if (ImGui::BeginPopupContextItem()) {
            if(selected_entity) {
                if (ImGui::MenuItem("Create Children Entity")) {
                    Entity created_entity = scene->create_entity("Empty Entity");

                    if(selected_entity) {
                        selected_entity.get_component<RelationshipComponent>().children.push_back(created_entity);
                        created_entity.get_component<RelationshipComponent>().parent = selected_entity;
                    }
                }
            }

			if (ImGui::MenuItem("Delete Entity")) {
				entity_deleted = true;
            }

			ImGui::EndPopup();
		}

        if (selected) {
            ImGui::PopStyleColor(2);
        }

        if (opened) {
            for (auto _child : relationship_component.children) {
                Entity child = { _child, scene.get() };
                auto& rc = child.get_component<RelationshipComponent>();
                tree(child, rc, iteration + 1);
            }

            ImGui::TreePop();
        }

        if (entity_deleted) {
			scene->destroy_entity(entity);
			if (selected_entity == entity) {
				selected_entity = {};
            }
		}


        if(iteration == 0) { ImGui::Separator(); }
    }

    void SceneHiearchyPanel::draw() {
        ImGui::Begin("Scene Hiearchy");

        scene->iterate([&](Entity entity){ 
            auto& rc = entity.get_component<RelationshipComponent>();
            if(rc.parent == entt::null) {
                tree(entity, rc, 0);
            }
        });

        if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
            selected_entity = {};

        // Right-click on blank space
        if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight)) {
            if (ImGui::MenuItem("Create Empty Entity")) {
                Entity created_entity = scene->create_entity("Empty Entity");

                if(selected_entity) {
                    selected_entity.get_component<RelationshipComponent>().children.push_back(created_entity);
                    created_entity.get_component<RelationshipComponent>().parent = selected_entity;
                }
            }

            ImGui::EndPopup();
        }

        ImGui::End();

        ImGui::Begin("Entity Properties");

        if(selected_entity) {
            if(selected_entity.has_component<UUIDComponent>()) {
                draw_component<UUIDComponent>(selected_entity, "UUID Component");
            }

            if(selected_entity.has_component<TagComponent>()) {
                draw_component<TagComponent>(selected_entity, "Tag Component");
            }

            if(selected_entity.has_component<TransformComponent>()) {
                draw_component<TransformComponent>(selected_entity, "Transform Component");
            }

            if(selected_entity.has_component<CameraComponent>()) {
                draw_component<CameraComponent>(selected_entity, "Camera Component");
            }

            if(selected_entity.has_component<ModelComponent>()) {
                draw_component<ModelComponent>(selected_entity, "Model Component");
            }

            if(selected_entity.has_component<DirectionalLightComponent>()) {
                draw_component<DirectionalLightComponent>(selected_entity, "Directional Light Component");
            }

            if(selected_entity.has_component<PointLightComponent>()) {
                draw_component<PointLightComponent>(selected_entity, "Point Light Component");
            }

            if(selected_entity.has_component<SpotLightComponent>()) {
                draw_component<SpotLightComponent>(selected_entity, "Spot Light Component");
            }

            if(selected_entity.has_component<RigidBodyComponent>()) {
                draw_component<RigidBodyComponent>(selected_entity, "RigidBody Component");
            }

            if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight)) {
                add_new_component<TransformComponent>(selected_entity, "Add Transform Component");
                add_new_component<ModelComponent>(selected_entity, "Add Model Component");
                add_new_component<CameraComponent>(selected_entity, "Add Camera Component");
                add_new_component<RigidBodyComponent>(selected_entity, "Add RigidBody Component");

                if(!(selected_entity.has_component<DirectionalLightComponent>() || selected_entity.has_component<PointLightComponent>() || selected_entity.has_component<SpotLightComponent>())) {
                    add_new_component<DirectionalLightComponent>(selected_entity, "Add Directional Light Component");
                    add_new_component<PointLightComponent>(selected_entity, "Add Point Light Component");
                    add_new_component<SpotLightComponent>(selected_entity, "Add Spot Light Component");
                }

                ImGui::EndPopup();
            }
        }

        ImGui::End();
    }

    void SceneHiearchyPanel::set_context(const std::shared_ptr<Scene> &_scene) {
        scene = _scene;
    }
}