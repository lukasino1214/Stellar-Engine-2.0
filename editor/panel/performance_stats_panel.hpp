#pragma once

#include <core/types.hpp>

namespace Stellar {
    struct PerformanceStatsPanel {
        PerformanceStatsPanel() = default;
        ~PerformanceStatsPanel() = default;

        void draw();

        u32 fps = 0;
        f32 delta_time = 0.0f;
        f32 up_time = 0.0f;
    };
}