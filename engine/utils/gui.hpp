#pragma once

#include <core/types.hpp>
#include <glm/glm.hpp>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>

namespace Stellar {
    namespace GUI {
        void generate_ID();

        void external_push_ID();
        void external_pop_ID();

        void push_deactivated_status();
        void pop_deactivated_status();

        void indent(f32 width, f32 height);

        void show_tooltip(const char* tooltip);
        auto rounded_button(const char* label, const ImVec2 &given_size, ImDrawFlags rounding_type, f32 rounding = GImGui->Style.FrameRounding, ImGuiButtonFlags flags = 0) -> bool;

        void begin_properties(ImGuiTableFlags tableFlags = ImGuiTableFlags_None);
        void end_properties();

        void begin_property(const char* label);
        void end_property();

        auto string_input(const char* label_ID, std::string &value, ImGuiInputTextFlags input_flags = ImGuiInputTextFlags_None) -> bool;
        auto u64_input(const char* label_ID, u64 &value, bool can_drag = false, ImGuiInputTextFlags input_flags = ImGuiInputTextFlags_None) -> bool;
        auto Vector3Input(glm::vec3 &value, const f32 *reset_values, const char** tooltips = nullptr) -> bool;

        auto string_property(const char* label, std::string &value, const char* tooltip = nullptr, ImGuiInputTextFlags input_flags = ImGuiInputTextFlags_None) -> bool;
        auto u64_property(const char* label, u64 &value, const char* tooltip = nullptr, ImGuiInputTextFlags input_flags = ImGuiInputTextFlags_None) -> bool;
        auto vec3_property(const char* label, glm::vec3 &value, const f32 *reset_values, const char** tooltips = nullptr) -> bool;
    }
}