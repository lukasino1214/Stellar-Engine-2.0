#include "window.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_NATIVE_INCLUDE_NONE
using HWND = void *;
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_WAYLAND
#endif
#include <GLFW/glfw3native.h>

#include <iostream>
#include <utility>

static std::atomic_int window_count = 0;

namespace Stellar {
    Window::Window(i32 _width, i32 _height, const std::string& _name) : width{_width}, height{_height}, name{_name}, glfw_window_ptr{nullptr} {
        open();
    }

    Window::~Window() {
        close();
    }

    auto Window::should_close() const -> bool {
        if (glfw_window_ptr == nullptr) { return false; }
        return static_cast<bool>(glfwWindowShouldClose(this->glfw_window_ptr));
    }

    auto Window::is_valid() const -> bool {
        return glfw_window_ptr != nullptr;
    }

    void Window::set_size(i32 x, i32 y) {
        glfwSetWindowSize(glfw_window_ptr, x, y);
        width = x;
        height = y;
    }

    void Window::set_position(i32 x, i32 y) {
        glfwSetWindowPos(glfw_window_ptr, x, y);
    }

    void Window::set_mouse_position(i32 x, i32 y) {
        glfwSetCursorPos(glfw_window_ptr, static_cast<f64>(x), static_cast<f64>(y));
    }

    void Window::set_mouse_capture(bool should_capture) {
        //glfwSetCursorPos(glfw_window_ptr, static_cast<f64>(width / 2), static_cast<f64>(height / 2));
        glfwSetInputMode(glfw_window_ptr, GLFW_CURSOR, should_capture ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        glfwSetInputMode(glfw_window_ptr, GLFW_RAW_MOUSE_MOTION, should_capture);
    }

    void Window::set_name(const std::string_view &_name) {
        glfwSetWindowTitle(glfw_window_ptr, _name.data());
        name = _name;
    }

    void Window::toggle_border(bool value) {
        glfwSetWindowAttrib(glfw_window_ptr, GLFW_DECORATED, static_cast<i32>(value));
    }

    void Window::open() {
        if(window_count == 0) { glfwInit(); }
        window_count.fetch_add(1);
        
        glfw_window_ptr = glfwCreateWindow(width, height, name.data(), nullptr, nullptr);
    }

    void Window::close() {
        if (glfw_window_ptr == nullptr) { return; }
        glfwDestroyWindow(glfw_window_ptr);
        glfw_window_ptr = nullptr;
        
        window_count.fetch_sub(1);
        if(window_count == 0) { glfwTerminate(); }
    }

    auto Window::get_native_platform() -> daxa::NativeWindowPlatform {
        switch(glfwGetPlatform()) {
            case GLFW_PLATFORM_WIN32: return daxa::NativeWindowPlatform::WIN32_API;
            case GLFW_PLATFORM_X11: return daxa::NativeWindowPlatform::XLIB_API;
            case GLFW_PLATFORM_WAYLAND: return daxa::NativeWindowPlatform::WAYLAND_API;
            default: return daxa::NativeWindowPlatform::UNKNOWN;
        }
    }

    auto Window::get_native_handle() -> daxa::NativeWindowHandle {
#if defined(_WIN32)
        return glfwGetWin32Window(glfw_window_ptr);
#elif defined(__linux__)
        switch (get_native_platform()) {
            case daxa::NativeWindowPlatform::WAYLAND_API:
                return reinterpret_cast<daxa::NativeWindowHandle>(glfwGetWaylandWindow(glfw_window_ptr));
            case daxa::NativeWindowPlatform::XLIB_API:
            default:
                return reinterpret_cast<daxa::NativeWindowHandle>(glfwGetX11Window(glfw_window_ptr));
        }
#endif
    }
}