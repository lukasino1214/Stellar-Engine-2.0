#include "performance_stats_panel.hpp"

#include <utils/gui.hpp>

namespace Stellar {
    void PerformanceStatsPanel::draw() {
        ImGui::Begin("Performance Stats");

        ImGui::Text("Frame Time: %f ms", static_cast<f64>(delta_time * 1000.0f));

        static constexpr u32 SAMPLE_COUNT = 200;
        static constexpr u32 REFRESH_RATE = 60;

        static u64 current_sample_index = 0;

        static std::array<f32, SAMPLE_COUNT> draw_time_samples = {};

        static f32 refresh_time = up_time;
        while (refresh_time < up_time) {
            draw_time_samples[current_sample_index] = delta_time * 1000.0f;

            current_sample_index = (current_sample_index + 1) % SAMPLE_COUNT;
            refresh_time += 1.0f / REFRESH_RATE;
        }

        float average_draw_time = 0.0f;
        for (int i = SAMPLE_COUNT - 1; i >= 0; i--) { average_draw_time += draw_time_samples[i]; }
        average_draw_time /= static_cast<f32>(SAMPLE_COUNT);

        std::array<char, 32> draw_time_overlay = {};
        snprintf(draw_time_overlay.data(), 32, "Average: %f ms", static_cast<f64>(average_draw_time));
        ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionWidth());
        ImGui::PlotLines("Lines", draw_time_samples.data(), SAMPLE_COUNT, static_cast<i32>(current_sample_index), draw_time_overlay.data(), 0.0f, 100.0f, ImVec2(0, 80.0f));

        ImGui::End();
    }
}