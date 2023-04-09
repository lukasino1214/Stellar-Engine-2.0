#pragma once 
#include <core/types.hpp>

#include <daxa/daxa.hpp>
#include "scene_hiearchy_panel.hpp"
#include <graphics/camera.hpp>
#include <core/window.hpp>

namespace Stellar {
    struct ViewportPanel {
        ViewportPanel() = default;
        ~ViewportPanel() = default;

        void draw(daxa::ImageId image, const std::shared_ptr<Window>& window, const std::unique_ptr<SceneHiearchyPanel>& scene_hiearchy_panel, ControlledCamera3D& camera);

        glm::vec2 mouse_pos = { 0, 0 };

        u32 viewport_size_x = 800;
        u32 viewport_size_y = 600;
        bool should_play = false;
        bool should_resize = true;
        bool mouse_pressed = false;
        i32 gizmo_type = 0;
    };
}