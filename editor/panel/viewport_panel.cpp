#include "viewport_panel.hpp"

#include <utils/gui.hpp>
#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/gtx/matrix_decompose.hpp>

#define NDEBUG true
#include <PxPhysicsAPI.h>
#include <physics/physics.hpp>

namespace Stellar {
    void ViewportPanel::draw(daxa::ImageId image, const std::shared_ptr<Window>& window, const std::unique_ptr<SceneHiearchyPanel> &scene_hiearchy_panel, ControlledCamera3D &camera) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Viewport");
        ImVec2 v = ImGui::GetContentRegionAvail();
        if(v.x != static_cast<f32>(viewport_size_x) || v.y != static_cast<f32>(viewport_size_y)) {
            viewport_size_x = static_cast<u32>(v.x);
            viewport_size_y = static_cast<u32>(v.y);
            should_resize = true;

            camera.camera.resize(static_cast<i32>(v.x), static_cast<i32>(v.y));
        }

        ImGui::Image(*reinterpret_cast<ImTextureID const *>(&image), v);

        if (glfwGetKey(window->glfw_window_ptr, GLFW_KEY_U) == GLFW_PRESS) {
            if (!ImGuizmo::IsUsing()) {
                gizmo_type = -1;
            }
        }

        if (glfwGetKey(window->glfw_window_ptr, GLFW_KEY_I) == GLFW_PRESS) {
            if (!ImGuizmo::IsUsing()) {
                gizmo_type = ImGuizmo::OPERATION::TRANSLATE;
            }
        }

        if (glfwGetKey(window->glfw_window_ptr, GLFW_KEY_O) == GLFW_PRESS) {
            if (!ImGuizmo::IsUsing()) {
                gizmo_type = ImGuizmo::OPERATION::ROTATE;
            }
        }

        if (glfwGetKey(window->glfw_window_ptr, GLFW_KEY_P) == GLFW_PRESS) {
            if (!ImGuizmo::IsUsing()) {
                gizmo_type = ImGuizmo::OPERATION::SCALE;
            }
        }

        Entity selected_entity = scene_hiearchy_panel->selected_entity;
        if (selected_entity && gizmo_type != -1 && selected_entity.has_component<TransformComponent>()) {
            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist();

            auto &tc = selected_entity.get_component<TransformComponent>();

            ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());

            glm::mat4 mod_mat = tc.model_matrix;
            glm::mat4 delta = glm::mat4{1.0};
            glm::mat4 proj = camera.camera.get_projection();
            proj[1][1] *= -1.0f;
            ImGuizmo::Manipulate(glm::value_ptr(camera.camera.get_view()), glm::value_ptr(proj), static_cast<ImGuizmo::OPERATION>(gizmo_type), ImGuizmo::LOCAL, glm::value_ptr(mod_mat), glm::value_ptr(delta), nullptr, nullptr, nullptr);

            if (ImGuizmo::IsUsing()) {
                glm::vec3 translation, rotation, scale;

                ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(mod_mat), &translation[0], &rotation[0], &scale[0]);

                tc.position += translation - tc.position;
                tc.rotation += rotation - tc.rotation;
                tc.scale += scale - tc.scale;
                tc.is_dirty = true;

                if(selected_entity.has_component<RigidBodyComponent>()) {
                    auto& pc = selected_entity.get_component<RigidBodyComponent>();
                    if(pc.body != nullptr) {
                        glm::quat a = glm::quat(glm::radians(tc.rotation));
                        physx::PxTransform transform;
                        transform.p = physx::PxVec3(tc.position.x, tc.position.y, tc.position.z),
                        transform.q = physx::PxQuat(a.x, a.y, a.z, a.w);

                        if(pc.rigid_body_type == RigidBodyType::Dynamic) {
                            pc.body->is<physx::PxRigidDynamic>()->setGlobalPose(transform);
                        } else {
                            pc.body->is<physx::PxRigidStatic>()->setGlobalPose(transform);
                        }
                    }
                }
            }
        }

        if(ImGui::IsItemHovered() && mouse_pressed && !ImGuizmo::IsUsing()) {

            ImVec2 w = ImGui::GetWindowPos();
            f32 center_x = w.x + (v.x / 2);
            f32 center_y = w.y + (v.y / 2);

            if(!should_play) {
                // hide cursor
                window->set_mouse_position(center_x, center_y);
                mouse_pos = { center_x, center_y };
            }

            auto offset = glm::vec2{mouse_pos.x - center_x, center_y - mouse_pos.y};
            camera.on_mouse_move(offset.x, offset.y);
            window->set_mouse_position(center_x, center_y);
            mouse_pos = { center_x, center_y };

            should_play = true;
        } else if(!mouse_pressed) {
            if(should_play) {
                // unhide cursor
            }

            should_play = false;
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }
}