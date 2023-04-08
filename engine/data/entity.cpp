#include "data/components.hpp"
#include "data/scene.hpp"
#include <data/entity.hpp>
#include <daxa/types.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <cstring>
#include "../../shaders/shared.inl"

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