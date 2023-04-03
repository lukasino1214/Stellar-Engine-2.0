#include "scene_hiearchy_panel.hpp"

#include <data/components.hpp>
#include <data/scene.hpp>

#include <imgui.h>
#include <imgui_internal.h>

#include <utils/gui.hpp>

namespace Stellar {
    template<typename T>
    inline void draw_component(Entity entity, const std::string_view& component_name) {
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
                component.draw();
                GUI::indent(0.0f, GImGui->Style.ItemSpacing.y);
                ImGui::TreePop();
            }

            if (remove_component) {
//                World::RemoveComponent<T>(entity);
            }
        }
    }

    SceneHiearchyPanel::SceneHiearchyPanel(const std::shared_ptr<Scene>& _scene) : scene{_scene} {}

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

        if (ImGui::IsItemClicked()) {
            selected_entity = entity;
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
        }

        ImGui::End();
    }

    void SceneHiearchyPanel::set_context(const std::shared_ptr<Scene> &_scene) {
        scene = _scene;
    }
}