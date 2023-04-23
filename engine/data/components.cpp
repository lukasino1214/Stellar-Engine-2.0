#include <data/components.hpp>

#include <cstring>

#include <memory>
#include <utils/gui.hpp>

#include <filesystem>
#include <data/entity.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Stellar {
    void UUIDComponent::draw() {
        GUI::begin_properties();

        GUI::push_deactivated_status();
        GUI::u64_property("UUID:", uuid.uuid, nullptr, ImGuiInputTextFlags_ReadOnly);
        GUI::pop_deactivated_status();

        GUI::end_properties();
    }

    void TagComponent::draw() {
        GUI::begin_properties();

        GUI::string_property("Tag:", name, nullptr, ImGuiInputTextFlags_None);
        
        GUI::end_properties();
    }

    void RelationshipComponent::draw() {

    }

    void TransformComponent::draw(Entity& entity) {
        GUI::begin_properties(ImGuiTableFlags_BordersInnerV);

        static std::array<f32, 3> reset_values = { 0.0f, 0.0f, 0.0f };
        static std::array<const char*, 3> tooltips = { "Some tooltip.", "Some tooltip.", "Some tooltip." };

        if (GUI::vec3_property("Position:", position, reset_values.data(), tooltips.data())) { is_dirty = true; }
        if (GUI::vec3_property("Rotation:", rotation, reset_values.data(), tooltips.data())) { is_dirty = true; }

        reset_values = { 1.0f, 1.0f, 1.0f };

        if (GUI::vec3_property("Scale:", scale, reset_values.data(), tooltips.data())) { is_dirty = true; }

        if(entity.has_component<RigidBodyComponent>() && is_dirty) {
            auto& pc = entity.get_component<RigidBodyComponent>();
            if(pc.body != nullptr) {
                glm::quat a = glm::quat(glm::radians(rotation));
                physx::PxTransform transform;
                transform.p = physx::PxVec3(position.x, position.y, position.z),
                transform.q = physx::PxQuat(a.x, a.y, a.z, a.w);

                if(pc.rigid_body_type == RigidBodyType::Dynamic) {
                    pc.body->is<physx::PxRigidDynamic>()->setGlobalPose(transform);
                } else {
                    pc.body->is<physx::PxRigidStatic>()->setGlobalPose(transform);
                }
            }
        }

        GUI::end_properties();
    }

    void CameraComponent::draw() {
        GUI::begin_properties();

        if(GUI::f32_property("FOV:", camera.fov, nullptr, ImGuiInputTextFlags_None)) { is_dirty = true; }
        if(GUI::f32_property("Aspect:", camera.aspect, nullptr, ImGuiInputTextFlags_None)) { is_dirty = true; }
        if(GUI::f32_property("Near Plane:", camera.near_clip, nullptr, ImGuiInputTextFlags_None)) { is_dirty = true; }
        if(GUI::f32_property("Far Plane:", camera.far_clip, nullptr, ImGuiInputTextFlags_None)) { is_dirty = true; }
        
        GUI::end_properties();
    }

    void ModelComponent::draw(daxa::Device device) {
        GUI::begin_properties();

        GUI::string_property("File Path:", file_path, nullptr, ImGuiInputTextFlags_ReadOnly);

        if(ImGui::Button("Load")) {
            if(std::filesystem::exists(file_path)) {
                model = std::make_shared<Model>(device, file_path);
            }
        }

        GUI::end_properties();
    }

    void DirectionalLightComponent::draw() {
        GUI::begin_properties();

        if (GUI::f32_property("Intensity:", intensity, nullptr)) { is_dirty = true;}
        if (ImGui::ColorPicker3("Color:", &color[0])) { is_dirty = true; }

        GUI::end_properties();
    }

    void PointLightComponent::draw() {
        GUI::begin_properties();

        if (GUI::f32_property("Intensity:", intensity, nullptr)) { is_dirty = true;}
        if (ImGui::ColorPicker3("Color:", &color[0])) { is_dirty = true; }

        GUI::end_properties();
    }

    void SpotLightComponent::draw() {
        GUI::begin_properties();

        static std::array<f32, 3> reset_values = { 0.0f, 0.0f, 0.0f };
        static std::array<const char*, 3> tooltips = { "Some tooltip.", "Some tooltip.", "Some tooltip." };

        if (GUI::f32_property("Intensity:", intensity, tooltips[0])) { is_dirty = true;}
        if (GUI::f32_property("Cut Off:", cut_off, tooltips[0])) { is_dirty = true;}
        if (GUI::f32_property("Outer Cut Off:", outer_cut_off, tooltips[0])) { is_dirty = true;}
        if (ImGui::ColorPicker3("Color:", &color[0])) { is_dirty = true; }

        GUI::end_properties();
    }

    void RigidBodyComponent::draw(const std::shared_ptr<Physics>& physics, Entity& entity) {
        GUI::begin_properties();

        bool something_changed = false;

        if(rigid_body_type == RigidBodyType::Dynamic) {
            if (GUI::f32_property("Density:", density, nullptr)) { something_changed = true; }
        }
        if (GUI::f32_property("Static Friction:", static_friction, nullptr)) { something_changed = true; }
        if (GUI::f32_property("Dynamic Friction:", dynamic_friction, nullptr)) { something_changed = true; }
        if (GUI::f32_property("Restitution:", restitution, nullptr)) { something_changed = true; }

        if(geometry_type == GeometryType::Sphere) {
            if (GUI::f32_property("Radius:", radius, nullptr)) { something_changed = true; }

        } else if (geometry_type == GeometryType::Capsule) {
            if (GUI::f32_property("Radius:", radius, nullptr)) { something_changed = true; }
            if (GUI::f32_property("Half Height:", half_height, nullptr)) { something_changed = true; }
        } else {
            if (GUI::vec3_property("Half Extent:", half_extent, nullptr, nullptr)) { something_changed = true; }
        }

        if(ImGui::BeginCombo("RigidBody Type", rigid_body_type == RigidBodyType::Static ? "static" : "dynamic")) {
            bool static_selected = rigid_body_type == RigidBodyType::Static;
            if (ImGui::Selectable("static", static_selected)) { rigid_body_type = RigidBodyType::Static; something_changed = true; }
            if(static_selected) { ImGui::SetItemDefaultFocus(); }

            bool dynamic_selected = rigid_body_type == RigidBodyType::Dynamic;
            if (ImGui::Selectable("dynamic", dynamic_selected)) { rigid_body_type = RigidBodyType::Dynamic; something_changed = true; }
            if(dynamic_selected) { ImGui::SetItemDefaultFocus(); }
            
            ImGui::EndCombo();
        }

        if(ImGui::BeginCombo("Geometry Type", geometry_type == GeometryType::Box ? "box" : (geometry_type == GeometryType::Sphere ? "sphere" : "capsule" ))) {
            bool box_selected = geometry_type == GeometryType::Box;
            if (ImGui::Selectable("box", box_selected)) { geometry_type = GeometryType::Box; something_changed = true; }
            if(box_selected) { ImGui::SetItemDefaultFocus(); }

            bool sphere_selected = geometry_type == GeometryType::Sphere;
            if (ImGui::Selectable("sphere", sphere_selected)) { geometry_type = GeometryType::Sphere; something_changed = true; }
            if(sphere_selected) { ImGui::SetItemDefaultFocus(); }

            bool capsule_selected = geometry_type == GeometryType::Capsule;
            if (ImGui::Selectable("capsule", capsule_selected)) { geometry_type = GeometryType::Capsule; something_changed = true; }
            if(capsule_selected) { ImGui::SetItemDefaultFocus(); }
            
            ImGui::EndCombo();
        }

        if(something_changed || body == nullptr) {
            if(body != nullptr) {
                physics->gScene->removeActor(*body);
                shape->release();
                material->release();
            }

            material = physics->gPhysics->createMaterial(static_friction, dynamic_friction, restitution);

            if(geometry_type == GeometryType::Sphere) {
                shape = physics->gPhysics->createShape(physx::PxSphereGeometry(radius), *material);
            } else if(geometry_type == GeometryType::Capsule) {
                shape = physics->gPhysics->createShape(physx::PxCapsuleGeometry(radius, half_height), *material);
            } else {
                shape = physics->gPhysics->createShape(physx::PxBoxGeometry(half_extent.x, half_extent.y, half_extent.z), *material);
            }

            auto& tc = entity.get_component<TransformComponent>();
            glm::quat a = glm::quat(glm::radians(tc.rotation));
            physx::PxTransform transform;
            transform.p = physx::PxVec3(tc.position.x, tc.position.y, tc.position.z),
            transform.q = physx::PxQuat(a.x, a.y, a.z, a.w);

            if(rigid_body_type == RigidBodyType::Static) {
                physx::PxRigidStatic* temp_body = physics->gPhysics->createRigidStatic(physx::PxTransform(physx::PxVec3(tc.position.x, tc.position.y, tc.position.z)));;
                temp_body->attachShape(*shape);
                temp_body->setGlobalPose(transform);
                body = temp_body->is<physx::PxActor>();
            } else {
                physx::PxRigidDynamic* temp_body = physics->gPhysics->createRigidDynamic(physx::PxTransform(physx::PxVec3(tc.position.x, tc.position.y, tc.position.z)));
                temp_body->attachShape(*shape);
                physx::PxRigidBodyExt::updateMassAndInertia(*temp_body, density);
                temp_body->setGlobalPose(transform);
                body = temp_body->is<physx::PxActor>();
            }

            physics->gScene->addActor(*body);
        }

        GUI::end_properties();
    } 
}