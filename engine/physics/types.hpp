#pragma once
#include <core/types.hpp>

namespace Stellar {
    enum struct RigidBodyType : u32 {
        Static = 0,
        Dynamic = 1,
    };

    enum struct GeometryType : u32 {
        Sphere = 0,
        Capsule = 1,
        Box = 2,
    };
}