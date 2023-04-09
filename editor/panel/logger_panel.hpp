#pragma once

#include <core/types.hpp>
#include <core/logger.hpp>
//#include <utils/gui.hpp>
#include <imgui.h>

namespace Stellar {
    struct LoggerPanel {
        std::array<char, 1024> input_buffer;
        std::vector<char *> items;
        std::vector<const char *> commands;
        std::vector<char *> history;
        int history_pos;
        ImGuiTextFilter filter;
        bool auto_scroll;
        bool scroll_to_bottom;

        LoggerPanel();
        ~LoggerPanel();

        static auto Stricmp(const char *s1, const char *s2) -> int;
        static auto Strnicmp(const char *s1, const char *s2, int n) -> int;
        static auto Strdup(const char *s) -> char *;
        static void Strtrim(char *s);

        void clear_log();
        void add_log(const char *fmt, ...) IM_FMTARGS(2);

        void draw(const char *title, bool *p_open);

        void exec_command(const char *command_line);
        
        auto on_text_edit(ImGuiInputTextCallbackData *data) -> int;
    };
}