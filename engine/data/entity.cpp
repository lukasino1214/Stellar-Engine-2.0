#include "data/components.hpp"
#include "data/scene.hpp"
#include <data/entity.hpp>
#include <daxa/types.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <cstring>
#include "../../shaders/shared.inl"
#define NDEBUG true
#include <PxPhysicsAPI.h>
#include <physics/physics.hpp>

namespace Stellar {
    Entity::Entity(entt::entity _handle, Scene* _scene) : handle{_handle}, scene{_scene} {}

    auto Entity::get_name() -> std::string_view {
        return get_component<TagComponent>().name;
    }

    auto Entity::get_uuid() -> UUID {
        return get_component<UUIDComponent>().uuid;
    }

    void Entity::update(daxa::Device& device) {
        if(has_component<CameraComponent>()) {
            auto& cc = get_component<CameraComponent>();
            if(cc.is_dirty) {
                cc.camera.proj_mat = glm::perspective(glm::radians(cc.camera.fov), cc.camera.aspect, cc.camera.near_clip, cc.camera.far_clip);
                cc.camera.proj_mat[1][1] *= 1.0f;

                cc.is_dirty = false;
            }
        }

        if(has_component<TransformComponent>()) {
            auto& tc = get_component<TransformComponent>();

            if(has_component<RigidBodyComponent>()) {
                auto& pc = get_component<RigidBodyComponent>();


                if(pc.body != nullptr && tc.is_dirty) {
                    glm::quat a = glm::quat(glm::radians(tc.rotation));
                    physx::PxTransform transform;
                    transform.p = physx::PxVec3(tc.position.x, tc.position.y, tc.position.z),
                    transform.q = physx::PxQuat(a.x, a.y, a.z, a.w);

                    if(pc.rigid_body_type == RigidBodyType::Dynamic) {
                        pc.body->is<physx::PxRigidDynamic>()->setGlobalPose(transform);
                    } else {
                        pc.body->is<physx::PxRigidStatic>()->setGlobalPose(transform);
                    }
                }

                if(pc.is_dirty || pc.body == nullptr) {
                    auto& physics = scene->physics;

                    if(pc.body != nullptr) {
                        physics->gScene->removeActor(*pc.body);
                        pc.shape->release();
                        pc.material->release();
                    }

                    pc.material = physics->gPhysics->createMaterial(pc.static_friction, pc.dynamic_friction, pc.restitution);

                    if(pc.geometry_type == GeometryType::Sphere) {
                        pc.shape = physics->gPhysics->createShape(physx::PxSphereGeometry(pc.radius), *pc.material);
                    } else if(pc.geometry_type == GeometryType::Capsule) {
                        pc.shape = physics->gPhysics->createShape(physx::PxCapsuleGeometry(pc.radius, pc.half_height), *pc.material);
                    } else {
                        pc.shape = physics->gPhysics->createShape(physx::PxBoxGeometry(pc.half_extent.x, pc.half_extent.y, pc.half_extent.z), *pc.material);
                    }

                    glm::quat a = glm::quat(glm::radians(tc.rotation));
                    physx::PxTransform transform;
                    transform.p = physx::PxVec3(tc.position.x, tc.position.y, tc.position.z),
                    transform.q = physx::PxQuat(a.x, a.y, a.z, a.w);

                    if(pc.rigid_body_type == RigidBodyType::Static) {
                        physx::PxRigidStatic* temp_body = physics->gPhysics->createRigidStatic(physx::PxTransform(physx::PxVec3(tc.position.x, tc.position.y, tc.position.z)));;
                        temp_body->attachShape(*pc.shape);
                        temp_body->setGlobalPose(transform);
                        pc.body = temp_body->is<physx::PxActor>();
                    } else {
                        physx::PxRigidDynamic* temp_body = physics->gPhysics->createRigidDynamic(physx::PxTransform(physx::PxVec3(tc.position.x, tc.position.y, tc.position.z)));
                        temp_body->attachShape(*pc.shape);
                        physx::PxRigidBodyExt::updateMassAndInertia(*temp_body, pc.density);
                        temp_body->setGlobalPose(transform);
                        pc.body = temp_body->is<physx::PxActor>();
                    }

                    physics->gScene->addActor(*pc.body);
                    pc.is_dirty = false;
                }

                physx::PxTransform t;
                if(pc.rigid_body_type == RigidBodyType::Dynamic) {
                    t = pc.body->is<physx::PxRigidDynamic>()->getGlobalPose();
                } else {
                    t = pc.body->is<physx::PxRigidStatic>()->getGlobalPose();
                }
                tc.position = { t.p.x, t.p.y, t.p.z };
                glm::vec3 rotation_radians = glm::eulerAngles(glm::quat{ t.q.w, t.q.x, t.q.y, t.q.z });
                tc.rotation = glm::degrees(rotation_radians);
                tc.is_dirty = true;
            }

            if(tc.is_dirty) {
                tc.model_matrix = glm::translate(glm::mat4(1.0f), tc.position) 
                    * glm::toMat4(glm::quat({glm::radians(tc.rotation.x), glm::radians(tc.rotation.y), glm::radians(tc.rotation.z)})) 
                    * glm::scale(glm::mat4(1.0f), tc.scale);

                tc.normal_matrix = glm::transpose(glm::inverse(tc.model_matrix));

                tc.is_dirty = false;

                TransformInfo tc_info = {
                    .model_matrix = *reinterpret_cast<daxa::f32mat4x4*>(&tc.model_matrix),
                    .normal_matrix = *reinterpret_cast<daxa::f32mat4x4*>(&tc.normal_matrix),
                };

                daxa::BufferId buffer = tc.transform_buffer;
                if(buffer.is_empty()) {
                    tc.transform_buffer = device.create_buffer({
                        .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
                        .size = sizeof(TransformInfo),
                        .debug_name = "transform buffer - " + std::to_string(get_component<UUIDComponent>().uuid.uuid),
                    });

                    buffer = tc.transform_buffer;
                }

                daxa::BufferId staging_transform_info_buffer = device.create_buffer(daxa::BufferInfo{
                    .memory_flags = daxa::MemoryFlagBits::HOST_ACCESS_RANDOM,
                    .size = sizeof(TransformInfo),
                    .debug_name = "",
                });

                auto cmd_list = device.create_command_list({
                    .debug_name = "",
                });

                cmd_list.destroy_buffer_deferred(staging_transform_info_buffer);

                auto* buffer_ptr = device.get_host_address_as<u32>(staging_transform_info_buffer);
                std::memcpy(buffer_ptr, &tc_info, sizeof(TransformInfo));

                cmd_list.copy_buffer_to_buffer({
                    .src_buffer = staging_transform_info_buffer,
                    .dst_buffer = buffer,
                    .size = sizeof(TransformInfo),
                });

                cmd_list.complete();
                device.submit_commands({
                    .command_lists = {std::move(cmd_list)},
                });

                if(has_component<CameraComponent>()) {
                    auto& cc = get_component<CameraComponent>();

                    cc.camera.set_pos(tc.position);
                    cc.camera.vrot_mat = glm::toMat4(glm::quat({glm::radians(tc.rotation.x), glm::radians(tc.rotation.y), glm::radians(tc.rotation.z)}));
                }
            }
        }

    }
}