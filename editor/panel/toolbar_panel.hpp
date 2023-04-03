#pragma once

#include <core/types.hpp>

namespace Stellar {
    enum class SceneState : u32 {
        Edit = 0, Play = 1, Simulate = 2
    };

    struct ToolbarPanel {
        ToolbarPanel() = default;
        ~ToolbarPanel() = default;

        void draw();

        SceneState scene_state = SceneState::Edit;
        bool play = false;
    };
}