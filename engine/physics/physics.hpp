#pragma once

#include <core/types.hpp>

#define NDEBUG true
#include <extensions/PxDefaultAllocator.h>
#include <extensions/PxDefaultErrorCallback.h>

namespace physx {
    struct PxFoundation;
    struct PxPhysics;
    struct PxDefaultCpuDispatcher;
    struct PxScene;
    struct PxPvd;
};

namespace Stellar {
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