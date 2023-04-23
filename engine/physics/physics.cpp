#include "physics.hpp"

namespace Stellar {
    Physics::Physics() {
        gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);

        gPvd = PxCreatePvd(*gFoundation);
        physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
        gPvd->connect(*transport,physx::PxPvdInstrumentationFlag::eALL);

        gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, physx::PxTolerancesScale(),true,gPvd);

        physx::PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
        sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
        gDispatcher = physx::PxDefaultCpuDispatcherCreate(2);
        sceneDesc.cpuDispatcher	= gDispatcher;
        sceneDesc.filterShader	= physx::PxDefaultSimulationFilterShader;
        gScene = gPhysics->createScene(sceneDesc);

        physx::PxPvdSceneClient* pvdClient = gScene->getScenePvdClient();
        if(pvdClient != nullptr)
        {
            pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
            pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
            pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
        }
    }

    Physics::~Physics() {
        gScene->release();
        gDispatcher->release();
        gPhysics->release();

        if(gPvd != nullptr) {
            physx::PxPvdTransport* transport = gPvd->getTransport();
            gPvd->release();	
            gPvd = nullptr;
            transport->release();
        }
        gFoundation->release();
    }

    void Physics::step(f32 delta_time) {
        gScene->simulate(delta_time);
        gScene->fetchResults(true);
    }
}