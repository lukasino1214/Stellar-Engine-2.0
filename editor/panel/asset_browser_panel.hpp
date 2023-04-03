#pragma once

#include <core/types.hpp>

#include <filesystem>

namespace Stellar {
    struct AssetBrowserPanel {
        explicit AssetBrowserPanel(const std::string_view& _project_directory);
        ~AssetBrowserPanel() = default;

        void draw();

        void tree(const std::filesystem::path& path);

        std::filesystem::path project_directory;
        std::filesystem::path current_directory;
    };
}