#include "logger_panel.hpp"
#include "core/types.hpp"

namespace Stellar {
/*struct ImGuiConsole {
    char input_buffer[256];
    std::vector<char *> items;
    std::vector<const char *> commands;
    std::vector<char *> history;
    int history_pos;
    ImGuiTextFilter filter;
    bool auto_scroll;
    bool scroll_to_bottom;*/

    ImGuiConsole::ImGuiConsole() : input_buffer{}, history_pos{-1}, auto_scroll{true}, scroll_to_bottom{false} {
        clear_log();
    }
    ImGuiConsole::~ImGuiConsole() {
        clear_log();
        for(usize i = 0; i < history.size(); i++) {
            std::free(history[i]);
        }
    }

    auto ImGuiConsole::Stricmp(const char *s1, const char *s2) -> int {
        int d = 0;
        while ((d = toupper(*s2) - toupper(*s1)) == 0 && (*s1 != 0)) {
            s1++;
            s2++;
        }
        return d;
    }
    auto ImGuiConsole::Strnicmp(const char *s1, const char *s2, int n) -> int {
        int d = 0;
        while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && (*s1 != 0)) {
            s1++;
            s2++;
            n--;
        }
        return d;
    }
    auto ImGuiConsole::Strdup(const char *s) -> char * {
        IM_ASSERT(s);
        size_t len = strlen(s) + 1;
        void *buf = malloc(len);
        IM_ASSERT(buf);
        return reinterpret_cast<char *>(std::memcpy(buf, reinterpret_cast<const void *>(s), len));
    }
    void ImGuiConsole::Strtrim(char *s) {
        char *str_end = s + strlen(s);
        while (str_end > s && str_end[-1] == ' ') {
            str_end--;
        }
        *str_end = 0;
    }
    void ImGuiConsole::clear_log() {
        for (usize i = 0; i < items.size(); i++) {
            std::free(items[i]);
        }
        items.clear();
    }
    void ImGuiConsole::add_log(const char *fmt, ...) {
        std::array<char, 1024> buf = {};
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf.data(), buf.size(), fmt, args);
        buf[buf.size() - 1] = 0;
        va_end(args);
        items.push_back(Strdup(buf.data()));
        // std::cout << buf << std::endl;
    }
    void ImGuiConsole::draw(const char *title, bool *p_open) {
        ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin(title, p_open)) { ImGui::End(); return; }
        if (ImGui::BeginPopupContextItem()) { if (ImGui::MenuItem("Close Console")) { *p_open = false; } ImGui::EndPopup(); }

        if (ImGui::SmallButton("Clear")) { clear_log(); }
        
        ImGui::SameLine();
        bool copy_to_clipboard = ImGui::SmallButton("Copy");
        ImGui::Separator();
        
        if (ImGui::BeginPopup("Options")) {
            ImGui::Checkbox("Auto-scroll", &auto_scroll);
            ImGui::EndPopup();
        }
        
        if (ImGui::Button("Options")) { ImGui::OpenPopup("Options"); }
        
        ImGui::SameLine();
        filter.Draw("Filter (\"incl,-excl\") (\"error\")", 180);
        ImGui::Separator();
        const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);
        
        if (ImGui::BeginPopupContextWindow()) {
            if (ImGui::Selectable("Clear")) { clear_log(); }
            ImGui::EndPopup();
        }
        
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));
        
        if (copy_to_clipboard) { ImGui::LogToClipboard(); }
        
        for (usize i = 0; i < Stellar::Logger::get_logs()->size(); i++) {
            auto& logs = *Stellar::Logger::get_logs();
            const char *item = logs[i].c_str();
            if (!filter.PassFilter(item)) { continue; }
            ImVec4 color;
            bool has_color = false;
            
            if (strstr(item, "[error]") != nullptr) {
                color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
                has_color = true;
            } else if (strncmp(item, "# ", 2) == 0) {
                color = ImVec4(1.0f, 0.8f, 0.6f, 1.0f);
                has_color = true;
            }

            if (has_color) { ImGui::PushStyleColor(ImGuiCol_Text, color); }
            ImGui::TextUnformatted(item);
            if (has_color) { ImGui::PopStyleColor(); }
        }

        for(auto *item : items) {
             if (!filter.PassFilter(item)) { continue; }
            ImVec4 color;
            bool has_color = false;
            if (strstr(item, "[error]") != nullptr) {
                color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
                has_color = true;
            } else if (strncmp(item, "# ", 2) == 0) {
                color = ImVec4(1.0f, 0.8f, 0.6f, 1.0f);
                has_color = true;
            }
            if (has_color) { ImGui::PushStyleColor(ImGuiCol_Text, color); }
            ImGui::TextUnformatted(item);
            if (has_color) { ImGui::PopStyleColor(); }
        }
        if (copy_to_clipboard) { ImGui::LogFinish(); }
        if (scroll_to_bottom || (auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())) { 
            ImGui::SetScrollHereY(1.0f);
        }

        scroll_to_bottom = false;
        ImGui::PopStyleVar();
        ImGui::EndChild();
        ImGui::Separator();
        bool reclaim_focus = false;
        ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
        
        if (ImGui::InputText(
                "Input", input_buffer.data(), input_buffer.size(), input_text_flags, [](ImGuiInputTextCallbackData *data) -> int {
                    auto* console = reinterpret_cast<ImGuiConsole *>(data->UserData);
                    return console->on_text_edit(data);
                },
                reinterpret_cast<void *>(this))) {
            char *s = input_buffer.data();
            Strtrim(s);
            if (s[0]) { exec_command(s); }
            s[0] = '\0';
            reclaim_focus = true;
        }
        ImGui::SetItemDefaultFocus();
        if (reclaim_focus) { ImGui::SetKeyboardFocusHere(-1); }
        ImGui::End();
    }
    void ImGuiConsole::exec_command(const char *command_line) {
        add_log("# %s\n", command_line);
        history_pos = -1;
        for (usize i = history.size() - 1; i >= 0; i--) {
            if (Stricmp(history[i], command_line) == 0) {
                std::free(history[i]);
                history.erase(history.begin() + i);
                break;
            }
        }
        history.push_back(Strdup(command_line));
        add_log("Unknown command: '%s'\n", command_line);
        scroll_to_bottom = true;
    }
    auto ImGuiConsole::on_text_edit(ImGuiInputTextCallbackData *data) -> int {
        switch (data->EventFlag) {
        case ImGuiInputTextFlags_CallbackCompletion: {
            const char *word_end = data->Buf + data->CursorPos;
            const char *word_start = word_end;
            while (word_start > data->Buf) {
                const char c = word_start[-1];
                if (c == ' ' || c == '\t' || c == ',' || c == ';') {
                    break;
                }

                word_start--;
            }
            ImVector<const char *> candidates;
            for (auto & command : commands) {
                if (Strnicmp(command, word_start, static_cast<i32>(word_end - word_start)) == 0) {
                    candidates.push_back(command);
                }
            }

            if (candidates.empty()) {
                add_log("No match for \"%.*s\"!\n", static_cast<i32>(word_end - word_start), word_start);
            } else if (candidates.size() == 1) {
                data->DeleteChars(static_cast<i32>(word_start - data->Buf), static_cast<i32>(word_end - word_start));
                data->InsertChars(data->CursorPos, candidates[0]);
                data->InsertChars(data->CursorPos, " ");
            } else {
                int match_len = static_cast<i32>(word_end - word_start);
                for (;;) {
                    int c = 0;
                    bool all_candidates_matches = true;
                    for (int i = 0; i < candidates.size() && all_candidates_matches; i++) {
                        if (i == 0) {
                            c = toupper(candidates[i][match_len]);
                        } else if (c == 0 || c != toupper(candidates[i][match_len])) {
                            all_candidates_matches = false;
                        }
                    }
                    
                    if (!all_candidates_matches) { break; }
                    match_len++;
                }
                if (match_len > 0) {
                    data->DeleteChars(static_cast<int>(word_start - data->Buf), static_cast<int>(word_end - word_start));
                    data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
                }
                add_log("Possible matches:\n");
                for (auto & candidate : candidates) {
                    add_log("- %s\n", candidate);
                }
            }
            break;
        }
        case ImGuiInputTextFlags_CallbackHistory: {
            const int prev_history_pos = history_pos;
            if (data->EventKey == ImGuiKey_UpArrow) {
                if (history_pos == -1) {
                    history_pos = static_cast<i32>(history.size()) - 1;
                } else if (history_pos > 0) {
                    history_pos--;
                }
            } else if (data->EventKey == ImGuiKey_DownArrow) {
                if (history_pos != -1) {
                    if (++history_pos >= static_cast<i32>(history.size())) {
                        history_pos = -1;
                    }
                }
            }
            if (prev_history_pos != history_pos) {
                const char *history_str = (history_pos >= 0) ? history[static_cast<usize>(history_pos)] : "";
                data->DeleteChars(0, data->BufTextLen);
                data->InsertChars(0, history_str);
            }
        }
        }
        return 0;
    }
/*};

static inline ImGuiConsole imgui_console;*/
}