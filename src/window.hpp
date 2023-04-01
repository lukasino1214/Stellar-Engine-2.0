#pragma once

#include <GLFW/glfw3.h>

#include <string_view>
#include <atomic>

#include <daxa/daxa.hpp>
using namespace daxa::types;

namespace Stellar {
    struct Window {
        Window(int _width, int _height, const std::string_view& _name);
        ~Window();

        [[nodiscard]] auto should_close() const -> bool;
        [[nodiscard]] auto is_valid() const -> bool;

        void set_size(i32 x, i32 y);
        void set_position(i32 x, i32 y);
        void set_name(const std::string_view& _name);

        void toggle_border(bool value);

        void close();
        void open();

        auto get_native_platform() -> daxa::NativeWindowPlatform;
        auto get_native_handle() -> daxa::NativeWindowHandle;

        int width;
        int height;
        std::string_view name;

        GLFWwindow* glfw_window_ptr;
    };
}