#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <atomic>

#include <core/types.hpp>
#include <daxa/daxa.hpp>

namespace Stellar {
    struct Window {
        Window(i32 _width, i32 _height, const std::string& _name);
        ~Window();

        [[nodiscard]] auto should_close() const -> bool;
        [[nodiscard]] auto is_valid() const -> bool;

        void set_size(i32 x, i32 y);
        void set_position(i32 x, i32 y);
        void set_mouse_position(i32 x, i32 y);
        void set_mouse_capture(bool should_capture);
        void set_name(const std::string_view& _name);

        void toggle_border(bool value);

        void close();
        void open();

        auto get_native_platform() -> daxa::NativeWindowPlatform;
        auto get_native_handle() -> daxa::NativeWindowHandle;

        i32 width;
        i32 height;
        std::string name;

        GLFWwindow* glfw_window_ptr;
    };
}