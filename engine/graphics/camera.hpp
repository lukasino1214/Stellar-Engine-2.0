#pragma once 

#include <core/types.hpp>

#include <GLFW/glfw3.h>

namespace Stellar {
    struct Camera3D {
        f32 fov = 90.0f, aspect = 1.0f;
        f32 near_clip = 0.1f, far_clip = 1000.0f;
        glm::mat4 proj_mat{1.0f};
        glm::mat4 vtrn_mat{1.0f};
        glm::mat4 vrot_mat{1.0f};

        void resize(i32 size_x, i32 size_y);

        void set_pos(const glm::vec3& _position);
        void set_rot(f32 x, f32 y);

        auto get_projection() -> glm::mat4;
        auto get_view() -> glm::mat4;
    };

    namespace input {
        struct Keybinds {
            i32 move_pz, move_nz;
            i32 move_px, move_nx;
            i32 move_py, move_ny;
            i32 toggle_pause;
            i32 toggle_sprint;
        };

        static inline constexpr Keybinds DEFAULT_KEYBINDS {
            .move_pz = GLFW_KEY_W,
            .move_nz = GLFW_KEY_S,
            .move_px = GLFW_KEY_A,
            .move_nx = GLFW_KEY_D,
            .move_py = GLFW_KEY_SPACE,
            .move_ny = GLFW_KEY_LEFT_CONTROL,
            .toggle_pause = GLFW_KEY_RIGHT_ALT,
            .toggle_sprint = GLFW_KEY_LEFT_SHIFT,
        };
    }

    struct ControlledCamera3D {
        Camera3D camera;
        input::Keybinds keybinds = input::DEFAULT_KEYBINDS;
        glm::vec3 position{ 0.0f, 0.0f, 0.0f };
        glm::vec3 velocity{ 0.0f, 0.0f, 0.0f };
        glm::vec3 rotation{ 0.0f, 0.0f, 0.0f };

        f32 speed = 30.0f, mouse_sens = 0.25f;
        f32 sprint_speed = 8.0f;
        f32 sin_rot_x = 0, cos_rot_x = 1;

        struct MoveFlags {
            uint8_t px : 1, py : 1, pz : 1, nx : 1, ny : 1, nz : 1, sprint : 1;
        } move{};

        void update(f32 dt);
        void on_key(i32 key, i32 action);
        void on_mouse_move(f32 delta_x, f32 delta_y);
    };
}