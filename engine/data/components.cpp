#include <data/components.hpp>

#include <cstring>

#include <memory>
#include <utils/gui.hpp>

#include <filesystem>
#include <data/entity.hpp>
#include <glm/gtx/quaternion.hpp>
#define todo() throw std::runtime_error("todo!");

#include <yaml-cpp/yaml.h>

#include <graphics/model.hpp>

namespace YAML {
    template<>
    struct convert<glm::vec3> {
        static Node encode(const glm::vec3 &rhs) {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.push_back(rhs.z);
            node.SetStyle(EmitterStyle::Flow);
            return node;
        }

        static bool decode(const Node &node, glm::vec3 &rhs) {
            if (!node.IsSequence() || node.size() != 3)
                return false;

            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            rhs.z = node[2].as<float>();
            return true;
        }
    };

    template<>
    struct convert<glm::vec4> {
        static Node encode(const glm::vec4 &rhs) {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.push_back(rhs.z);
            node.push_back(rhs.w);
            node.SetStyle(EmitterStyle::Flow);
            return node;
        }

        static bool decode(const Node &node, glm::vec4 &rhs) {
            if (!node.IsSequence() || node.size() != 4)
                return false;

            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            rhs.z = node[2].as<float>();
            rhs.w = node[3].as<float>();
            return true;
        }
    };
}

YAML::Emitter &operator<<(YAML::Emitter &out, const glm::vec3 &v) {
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
    return out;
}

YAML::Emitter &operator<<(YAML::Emitter &out, const glm::vec4 &v) {
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
    return out;
}

namespace Stellar {
    void UUIDComponent::draw() {
        GUI::begin_properties();

        GUI::push_deactivated_status();
        GUI::u64_property("UUID:", uuid.uuid, nullptr, ImGuiInputTextFlags_ReadOnly);
        GUI::pop_deactivated_status();

        GUI::end_properties();
    }

    void UUIDComponent::serialize(YAML::Emitter &out, Entity &entity) {
        todo();
    }

    void UUIDComponent::deserialize(YAML::Node &node, Entity &entity) {
        todo();
    }

    void TagComponent::draw() {
        GUI::begin_properties();

        GUI::string_property("Tag:", name, nullptr, ImGuiInputTextFlags_None);
        
        GUI::end_properties();
    }

    void TagComponent::serialize(YAML::Emitter &out, Entity &entity) {
        out << YAML::Key << "TagComponent";
        out << YAML::BeginMap;

        out << YAML::Key << "Tag" << YAML::Value << entity.get_component<TagComponent>().name;

        out << YAML::EndMap;
    }

    void TagComponent::deserialize(YAML::Node &node, Entity &entity) {
        todo();
    }

    void RelationshipComponent::draw() {

    }

    void RelationshipComponent::serialize(YAML::Emitter &out, Entity &entity) {
        out << YAML::Key << "RelationshipComponent";
        out << YAML::BeginMap;

        auto& rc = entity.get_component<RelationshipComponent>();
        Entity parent { rc.parent, entity.scene };
        u64 parent_uuid = parent ? parent.get_component<UUIDComponent>().uuid.uuid : 0;
        out << YAML::Key << "Parent" << YAML::Value << parent_uuid;
        
        out << YAML::Key << "Children" << YAML::Value << YAML::BeginSeq;

        for(auto& _child : rc.children) {
            Entity child { _child, entity.scene };
            if(!child) { continue; }
            u64 child_uuid = child ? child.get_component<UUIDComponent>().uuid.uuid : 0;

            out << YAML::Value << child_uuid;
        }

        out << YAML::EndSeq;

        out << YAML::EndMap;
    }

    void RelationshipComponent::deserialize(YAML::Node &node, Entity &entity) {
        todo();
    }

    void TransformComponent::draw() {
        GUI::begin_properties(ImGuiTableFlags_BordersInnerV);

        static std::array<f32, 3> reset_values = { 0.0f, 0.0f, 0.0f };
        static std::array<const char*, 3> tooltips = { "Some tooltip.", "Some tooltip.", "Some tooltip." };

        if (GUI::vec3_property("Position:", position, reset_values.data(), tooltips.data())) { is_dirty = true; }
        if (GUI::vec3_property("Rotation:", rotation, reset_values.data(), tooltips.data())) { is_dirty = true; }

        reset_values = { 1.0f, 1.0f, 1.0f };

        if (GUI::vec3_property("Scale:", scale, reset_values.data(), tooltips.data())) { is_dirty = true; }

        GUI::end_properties();
    }

    void TransformComponent::serialize(YAML::Emitter &out, Entity &entity) {
        out << YAML::Key << "TransformComponent";
        out << YAML::BeginMap;

        auto& tc = entity.get_component<TransformComponent>();
        out << YAML::Key << "Position" << YAML::Value << tc.position;
        out << YAML::Key << "Rotation" << YAML::Value << tc.rotation;
        out << YAML::Key << "Scale" << YAML::Value << tc.scale;

        out << YAML::EndMap;
    }

    void TransformComponent::deserialize(YAML::Node &node, Entity &entity) {
        auto &tc = entity.add_component<TransformComponent>();
        tc.position = node["Position"].as<glm::vec3>();
        tc.rotation = node["Rotation"].as<glm::vec3>();
        tc.scale = node["Scale"].as<glm::vec3>();
        tc.is_dirty = true;
    }

    void CameraComponent::draw() {
        GUI::begin_properties();

        if(GUI::f32_property("FOV:", camera.fov, nullptr, ImGuiInputTextFlags_None)) { is_dirty = true; }
        if(GUI::f32_property("Aspect:", camera.aspect, nullptr, ImGuiInputTextFlags_None)) { is_dirty = true; }
        if(GUI::f32_property("Near Plane:", camera.near_clip, nullptr, ImGuiInputTextFlags_None)) { is_dirty = true; }
        if(GUI::f32_property("Far Plane:", camera.far_clip, nullptr, ImGuiInputTextFlags_None)) { is_dirty = true; }
        
        GUI::end_properties();
    }

    void CameraComponent::serialize(YAML::Emitter &out, Entity &entity) {
        out << YAML::Key << "CameraComponent";
        out << YAML::BeginMap;

        auto& cc = entity.get_component<CameraComponent>();
        out << YAML::Key << "FOV" << YAML::Value << cc.camera.fov;
        out << YAML::Key << "Aspect" << YAML::Value << cc.camera.aspect;
        out << YAML::Key << "NearPlane" << YAML::Value << cc.camera.near_clip;
        out << YAML::Key << "FarPlane" << YAML::Value << cc.camera.far_clip;

        out << YAML::EndMap;
    }

    void CameraComponent::deserialize(YAML::Node &node, Entity &entity) {
        auto &cc = entity.add_component<CameraComponent>();
        cc.camera.fov = node["FOV"].as<f32>();
        cc.camera.aspect = node["Aspect"].as<f32>();
        cc.camera.near_clip = node["NearPlane"].as<f32>();
        cc.camera.far_clip = node["FarPlane"].as<f32>();
        cc.is_dirty = true;
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

    void ModelComponent::serialize(YAML::Emitter &out, Entity &entity) {
        out << YAML::Key << "ModelComponent";
        out << YAML::BeginMap;

        auto& mc = entity.get_component<ModelComponent>();
        out << YAML::Key << "Filepath" << YAML::Value << mc.file_path;

        out << YAML::EndMap;
    }

    void ModelComponent::deserialize(YAML::Node &node, Entity &entity, daxa::Device& device) {
        auto& mc = entity.add_component<ModelComponent>();
        mc.file_path = node["Filepath"].as<std::string>();
        mc.model = std::make_shared<Model>(device, mc.file_path);
    }

    void DirectionalLightComponent::draw() {
        GUI::begin_properties();

        if (GUI::f32_property("Intensity:", intensity, nullptr)) { is_dirty = true;}
        if (ImGui::ColorPicker3("Color:", &color[0])) { is_dirty = true; }

        GUI::end_properties();
    }

    void DirectionalLightComponent::serialize(YAML::Emitter &out, Entity &entity) {
        out << YAML::Key << "DirectionalLightComponent";
        out << YAML::BeginMap;

        auto& dlc = entity.get_component<DirectionalLightComponent>();
        out << YAML::Key << "Color" << YAML::Value << dlc.color;
        out << YAML::Key << "Intensity" << YAML::Value << dlc.intensity;

        out << YAML::EndMap;
    }

    void DirectionalLightComponent::deserialize(YAML::Node &node, Entity &entity) {
        auto& dlc = entity.add_component<DirectionalLightComponent>();
        dlc.color = node["Color"].as<glm::vec3>();
        dlc.intensity = node["Intensity"].as<f32>();
    }

    void PointLightComponent::draw() {
        GUI::begin_properties();

        if (GUI::f32_property("Intensity:", intensity, nullptr)) { is_dirty = true;}
        if (ImGui::ColorPicker3("Color:", &color[0])) { is_dirty = true; }

        GUI::end_properties();
    }

    void PointLightComponent::serialize(YAML::Emitter &out, Entity &entity) {
        out << YAML::Key << "PointLightComponent";
        out << YAML::BeginMap;

        auto& plc = entity.get_component<PointLightComponent>();
        out << YAML::Key << "Color" << YAML::Value << plc.color;
        out << YAML::Key << "Intensity" << YAML::Value << plc.intensity;

        out << YAML::EndMap;
    }

    void PointLightComponent::deserialize(YAML::Node &node, Entity &entity) {
        auto& plc = entity.add_component<PointLightComponent>();
        plc.color = node["Color"].as<glm::vec3>();
        plc.intensity = node["Intensity"].as<f32>();
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

    void SpotLightComponent::serialize(YAML::Emitter &out, Entity &entity) {
        out << YAML::Key << "SpotLightComponent";
        out << YAML::BeginMap;

        auto& slc = entity.get_component<SpotLightComponent>();
        out << YAML::Key << "Color" << YAML::Value << slc.color;
        out << YAML::Key << "Intensity" << YAML::Value << slc.intensity;
        out << YAML::Key << "CutOff" << YAML::Value << slc.cut_off;
        out << YAML::Key << "OuterCutOff" << YAML::Value << slc.outer_cut_off;

        out << YAML::EndMap;
    }

    void SpotLightComponent::deserialize(YAML::Node &node, Entity &entity) {
        auto& slc = entity.add_component<SpotLightComponent>();
        slc.color = node["Color"].as<glm::vec3>();
        slc.intensity = node["Intensity"].as<f32>();
        slc.cut_off = node["CutOff"].as<f32>();
        slc.outer_cut_off = node["OuterCutOff"].as<f32>();
    }

    void RigidBodyComponent::draw() {
        GUI::begin_properties();

        if(rigid_body_type == RigidBodyType::Dynamic) {
            if (GUI::f32_property("Density:", density, nullptr)) { is_dirty = true; }
        }
        if (GUI::f32_property("Static Friction:", static_friction, nullptr)) { is_dirty = true; }
        if (GUI::f32_property("Dynamic Friction:", dynamic_friction, nullptr)) { is_dirty = true; }
        if (GUI::f32_property("Restitution:", restitution, nullptr)) { is_dirty = true; }

        if(geometry_type == GeometryType::Sphere) {
            if (GUI::f32_property("Radius:", radius, nullptr)) { is_dirty = true; }

        } else if (geometry_type == GeometryType::Capsule) {
            if (GUI::f32_property("Radius:", radius, nullptr)) { is_dirty = true; }
            if (GUI::f32_property("Half Height:", half_height, nullptr)) { is_dirty = true; }
        } else {
            if (GUI::vec3_property("Half Extent:", half_extent, nullptr, nullptr)) { is_dirty = true; }
        }

        if(ImGui::BeginCombo("RigidBody Type", rigid_body_type == RigidBodyType::Static ? "static" : "dynamic")) {
            bool static_selected = rigid_body_type == RigidBodyType::Static;
            if (ImGui::Selectable("static", static_selected)) { rigid_body_type = RigidBodyType::Static; is_dirty = true; }
            if(static_selected) { ImGui::SetItemDefaultFocus(); }

            bool dynamic_selected = rigid_body_type == RigidBodyType::Dynamic;
            if (ImGui::Selectable("dynamic", dynamic_selected)) { rigid_body_type = RigidBodyType::Dynamic; is_dirty = true; }
            if(dynamic_selected) { ImGui::SetItemDefaultFocus(); }
            
            ImGui::EndCombo();
        }

        if(ImGui::BeginCombo("Geometry Type", geometry_type == GeometryType::Box ? "box" : (geometry_type == GeometryType::Sphere ? "sphere" : "capsule" ))) {
            bool box_selected = geometry_type == GeometryType::Box;
            if (ImGui::Selectable("box", box_selected)) { geometry_type = GeometryType::Box; is_dirty = true; }
            if(box_selected) { ImGui::SetItemDefaultFocus(); }

            bool sphere_selected = geometry_type == GeometryType::Sphere;
            if (ImGui::Selectable("sphere", sphere_selected)) { geometry_type = GeometryType::Sphere; is_dirty = true; }
            if(sphere_selected) { ImGui::SetItemDefaultFocus(); }

            bool capsule_selected = geometry_type == GeometryType::Capsule;
            if (ImGui::Selectable("capsule", capsule_selected)) { geometry_type = GeometryType::Capsule; is_dirty = true; }
            if(capsule_selected) { ImGui::SetItemDefaultFocus(); }
            
            ImGui::EndCombo();
        }
        
        GUI::end_properties();
    } 

    void RigidBodyComponent::serialize(YAML::Emitter &out, Entity &entity) {
        todo();
    }

    void RigidBodyComponent::deserialize(YAML::Node &node, Entity &entity) {
        todo();
    }
}