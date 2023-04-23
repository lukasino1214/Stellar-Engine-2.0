#pragma once

#include <core/types.hpp>
#define NDEBUG true
#include <PxPhysicsAPI.h>

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

    struct Physics {
        Physics();
        ~Physics();

        void step(f32 delta_time);

        physx::PxDefaultAllocator gAllocator;
        physx::PxDefaultErrorCallback gErrorCallback;

        physx::PxFoundation* gFoundation = nullptr;
        physx::PxPhysics* gPhysics = nullptr;

        physx::PxDefaultCpuDispatcher* gDispatcher = nullptr;
        physx::PxScene*	gScene = nullptr;

        physx::PxPvd* gPvd = nullptr;
    };
}