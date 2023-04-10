#include "toolbar_panel.hpp"

#include <string_view>
#include <utils/gui.hpp>

#include <iostream>

namespace Stellar {
    ToolbarPanel::ToolbarPanel(const std::shared_ptr<Window>& _window) : window{_window} {}

    void ToolbarPanel::draw() {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        auto &colors = ImGui::GetStyle().Colors;
        const auto &buttonHovered = colors[ImGuiCol_ButtonHovered];
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(buttonHovered.x, buttonHovered.y, buttonHovered.z, 0.5f));
        const auto &buttonActive = colors[ImGuiCol_ButtonActive];
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(buttonActive.x, buttonActive.y, buttonActive.z, 0.5f));

        ImGui::Begin("##Toolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        bool toolbarEnabled = play;

        float size = ImGui::GetWindowHeight() - 4.0f;
        ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) - (size * 0.5f));

        /*bool hasPlayButton = scene_state == SceneState::Edit || scene_state == SceneState::Play;
        bool hasSimulateButton = scene_state == SceneState::Edit || scene_state == SceneState::Simulate;
        bool hasPauseButton = scene_state != SceneState::Edit;*/

        /*if (hasPlayButton) {
            std::string_view action = (scene_state == SceneState::Edit) ? "play" : "stop";
            if (ImGui::Button(action.data(), ImVec2(size * 2.5f, size)) && toolbarEnabled) {
                if (scene_state == SceneState::Edit || scene_state == SceneState::Simulate) {
                    play = true;
                }
                else if (scene_state == SceneState::Play) {
                    play = false;
                }
            }
        }*/

        if(scene_state == SceneState::Edit) {
            if (ImGui::Button("play", ImVec2(size * 2.5f, size))) {
                scene_state = SceneState::Play;
                //window->set_mouse_capture(true);
            }
        }

        if(scene_state == SceneState::Play) {
            if (ImGui::Button("edit", ImVec2(size * 2.5f, size))) {
                scene_state = SceneState::Edit;
                //window->set_mouse_capture(false);
            }
        }

        /*if (hasSimulateButton) {
            if (hasPlayButton)
                ImGui::SameLine();

            std::string_view action = (scene_state == SceneState::Edit || scene_state == SceneState::Simulate) ? "simulate" : "stop";
            if (ImGui::Button(action.data(), ImVec2(size * 2.5f, size)) && toolbarEnabled) {
                if (scene_state == SceneState::Edit || scene_state == SceneState::Play) {
                    play = true;
                }

                else if (scene_state == SceneState::Simulate) {
                    play = false;
                }
            }
        }
        if (hasPauseButton) {
            ImGui::SameLine();
            {
                if (ImGui::Button("pause", ImVec2(size * 2.5f, size)) && toolbarEnabled) {
                    play = !play;
                }
            }

            // Step button
            if (play) {
                ImGui::SameLine();
                {
                    if (ImGui::Button("unpause", ImVec2(size * 2.5f, size)) && toolbarEnabled) {}
                }
            }
        }*/
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(3);
        ImGui::End();
    }
} // namespace Stellar