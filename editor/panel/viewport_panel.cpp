#include "viewport_panel.hpp"

#include <utils/gui.hpp>
#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/gtx/matrix_decompose.hpp>

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

                auto decompose_transform = [](const glm::mat4 &transform, glm::vec3 &translation, glm::vec3 &rotation, glm::vec3 &scale) -> bool {
                    using namespace glm;
                    using T = float;

                    mat4 local_matrix(transform);

                    if (epsilonEqual(local_matrix[3][3], static_cast<float>(0), epsilon<T>()))
                        return false;

                    if (epsilonNotEqual(local_matrix[0][3], static_cast<T>(0), epsilon<T>()) || epsilonNotEqual(local_matrix[1][3], static_cast<T>(0), epsilon<T>()) || epsilonNotEqual(local_matrix[2][3], static_cast<T>(0), epsilon<T>())) {
                        local_matrix[0][3] = local_matrix[1][3] = local_matrix[2][3] = static_cast<T>(0);
                        local_matrix[3][3] = static_cast<T>(1);
                    }

                    translation = vec3(local_matrix[3]);
                    local_matrix[3] = vec4(0, 0, 0, local_matrix[3].w);

                    vec3 row[3], Pdum3;

                    for (length_t i = 0; i < 3; ++i)
                        for (length_t j = 0; j < 3; ++j)
                            row[i][j] = local_matrix[i][j];

                    scale.x = length(row[0]);
                    row[0] = glm::detail::scale(row[0], static_cast<T>(1));
                    scale.y = length(row[1]);
                    row[1] = glm::detail::scale(row[1], static_cast<T>(1));
                    scale.z = length(row[2]);
                    row[2] = glm::detail::scale(row[2], static_cast<T>(1));

                    rotation.y = asin(-row[0][2]);
                    if (cos(rotation.y) != 0) {
                        rotation.x = atan2(row[1][2], row[2][2]);
                        rotation.z = atan2(row[0][1], row[0][0]);
                    } else {
                        rotation.x = atan2(-row[2][0], row[1][1]);
                        rotation.z = 0;
                    }

                    return true;
                };

                decompose_transform(mod_mat, translation, rotation, scale);

                tc.position += translation - tc.position;
                tc.rotation += rotation - tc.rotation;
                tc.scale += scale - tc.scale;
                tc.is_dirty = true;
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