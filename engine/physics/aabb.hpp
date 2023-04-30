#pragma once

#include <glm/glm.hpp>

namespace Stellar {
    struct AABB {
        glm::vec3 min;
        glm::vec3 max;
    };
}