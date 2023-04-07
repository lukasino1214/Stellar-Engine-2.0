#pragma once

#include <core/types.hpp>
#include <core/window.hpp>

namespace Stellar {
    enum class SceneState : u32 {
        Edit = 0, Play = 1, Simulate = 2
    };

    struct ToolbarPanel {
        ToolbarPanel(const std::shared_ptr<Window>& _window);
        ~ToolbarPanel() = default;

        void draw();

        SceneState scene_state = SceneState::Edit;
        bool play = false;
        std::shared_ptr<Window> window;
    };
}