#include "data/components.hpp"
#include "data/scene.hpp"
#include <data/entity.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <iostream>

namespace Stellar {
    Entity::Entity(entt::entity _handle, Scene* _scene) : handle{_handle}, scene{_scene} {}

    auto Entity::get_name() -> std::string_view {
        return get_component<TagComponent>().name;
    }

    auto Entity::get_uuid() -> UUID {
        return get_component<UUIDComponent>().uuid;
    }

    void Entity::update() {
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

                if(has_component<CameraComponent>()) {
                    auto& cc = get_component<CameraComponent>();

                    cc.camera.set_pos(tc.position);
                    cc.camera.vrot_mat = glm::toMat4(glm::quat({glm::radians(tc.rotation.x), glm::radians(tc.rotation.y), glm::radians(tc.rotation.z)}));
                }
            }
        }

    }
}