#include "asset_browser_panel.hpp"

#include <filesystem>

#include <utils/gui.hpp>
#include <utils/utils.hpp>

#include <iostream>

namespace Stellar {
    AssetBrowserPanel::AssetBrowserPanel(const std::string_view& _project_directory) : project_directory{_project_directory}, current_directory{_project_directory} {}

    void AssetBrowserPanel::tree(const std::filesystem::path &path) {
        ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanAvailWidth;

        bool opened = ImGui::TreeNodeEx(path.filename().string().c_str(), treeNodeFlags);

        if (ImGui::IsItemClicked()) {
            current_directory = path;
        }

        if (opened) {
            for(const auto& directory_entry : std::filesystem::directory_iterator(path)) {
                if(directory_entry.is_directory()) {
                    tree(directory_entry.path()); 
                }
            }

            ImGui::TreePop();
        }
    }

    void AssetBrowserPanel::draw() {
        ImGui::Begin("Asset Tree");
        tree(project_directory);
        ImGui::End();

        ImGui::Begin("Asset Browser");
        if (current_directory != project_directory) {
			if (ImGui::Button("<-")) {
				current_directory = current_directory.parent_path();
			}
		}

		f32 cell_size = 80.0f + 16.0f;
		f32 panelWidth = ImGui::GetContentRegionAvail().x;
		i32 column_count = static_cast<int>(panelWidth / cell_size);

		if (column_count < 1) { column_count = 1; }

		ImGui::Columns(column_count, 0, false);

		for (const auto& directory_entry : std::filesystem::directory_iterator(current_directory)) {
			const auto& path = directory_entry.path();
			std::string filename_string = path.filename().string();
			
			ImGui::PushID(filename_string.c_str());
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::Button(filename_string.c_str(), { 80.0f, 80.0f });

			ImGui::PopStyleColor();
			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
				if (directory_entry.is_directory()) {
					current_directory /= path.filename();
                }

			}

			ImGui::NextColumn();

			ImGui::PopID();
		}
        ImGui::End();

    }
}