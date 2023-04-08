#include "core/uuid.hpp"
#include <data/components.hpp>
#include <data/scene.hpp>

#include <data/entity.hpp>
#include <entt/entity/entity.hpp>
#include <stdexcept>
#include <string>
#include <yaml-cpp/emittermanip.h>
#include <yaml-cpp/yaml.h>

#include <fstream>
#include <iostream>
#include <cstring>

#include "../../shaders/shared.inl"

#include <glm/gtx/rotate_vector.hpp>


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
    Scene::Scene(const std::string_view& _name, daxa::Device _device) : name{_name}, device{_device} {
        registry = std::make_unique<entt::registry>();

        light_buffer = device.create_buffer({
            .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
            .size = sizeof(LightBuffer),
            .debug_name = "light buffer",
        });
    }

    Scene::~Scene() {
        iterate([&](Entity entity) {
            if(entity.has_component<TransformComponent>()) {
                auto& tc = entity.get_component<TransformComponent>();
                if(!tc.transform_buffer.is_empty()) {
                    device.destroy_buffer(tc.transform_buffer);
                }
            }
        });

        device.destroy_buffer(light_buffer);
    }

    auto Scene::create_entity(const std::string_view& _name) -> Entity {
        return create_entity_with_UUID(_name, UUID());
    }

    auto Scene::create_entity_with_UUID(const std::string_view& _name, UUID _uuid) -> Entity {
        Entity entity = Entity(registry->create(), this);

        entity.add_component<UUIDComponent>().uuid = _uuid;
        entity.add_component<TagComponent>().name = _name;
        entity.add_component<RelationshipComponent>();

        return entity;
    }

    void Scene::destroy_entity(const Entity& entity) {
        const auto& rc = entity.get_component<RelationshipComponent>();
        for(auto _children : rc.children) {
            destroy_entity(Entity { _children, this });
        }

        registry->destroy(entity.handle);
    }

    void Scene::iterate(std::function<void(Entity)> fn) {
        registry->each([&](auto entity_handle) {
            Entity entity = {entity_handle, this};
            if (!entity) { return; }
            if(!entity.has_component<TagComponent>()) { return; } // stupid fix I have no idea why its happening

            fn(entity);
        });
    };

    void Scene::serialize(const std::string_view& path) {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Scene" << YAML::Value << name;
        out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

        registry->each([&](auto entity_handle) {
			Entity entity = { entity_handle, this };
			if (!entity) {
				return;
            }

            out << YAML::BeginMap;
            out << YAML::Key << "Entity" << YAML::Value << entity.get_uuid();
            if(entity.has_component<TagComponent>()) {
                out << YAML::Key << "TagComponent";
                out << YAML::BeginMap;

                out << YAML::Key << "Tag" << YAML::Value << entity.get_component<TagComponent>().name;

                out << YAML::EndMap;
            }

            if(entity.has_component<RelationshipComponent>()) {
                out << YAML::Key << "RelationshipComponent";
                out << YAML::BeginMap;

                auto& rc = entity.get_component<RelationshipComponent>();
                Entity parent { rc.parent, this };
                u64 parent_uuid = parent ? parent.get_component<UUIDComponent>().uuid.uuid : 0;
                out << YAML::Key << "Parent" << YAML::Value << parent_uuid;
                
                out << YAML::Key << "Children" << YAML::Value << YAML::BeginSeq;

                for(auto& _child : rc.children) {
                    Entity child { _child, this };
                    if(!child) { continue; }
                    u64 child_uuid = child ? child.get_component<UUIDComponent>().uuid.uuid : 0;

                    out << YAML::Value << child_uuid;
                }

                out << YAML::EndSeq;

                out << YAML::EndMap;
            }

            if(entity.has_component<TransformComponent>()) {
                out << YAML::Key << "TransformComponent";
                out << YAML::BeginMap;

                auto& tc = entity.get_component<TransformComponent>();
                out << YAML::Key << "Position" << YAML::Value << tc.position;
                out << YAML::Key << "Rotation" << YAML::Value << tc.rotation;
                out << YAML::Key << "Scale" << YAML::Value << tc.scale;

                out << YAML::EndMap;
            }

            if(entity.has_component<CameraComponent>()) {
                out << YAML::Key << "CameraComponent";
                out << YAML::BeginMap;

                auto& cc = entity.get_component<CameraComponent>();
                out << YAML::Key << "FOV" << YAML::Value << cc.camera.fov;
                out << YAML::Key << "Aspect" << YAML::Value << cc.camera.aspect;
                out << YAML::Key << "NearPlane" << YAML::Value << cc.camera.near_clip;
                out << YAML::Key << "FarPlane" << YAML::Value << cc.camera.far_clip;

                out << YAML::EndMap;
            }

            if(entity.has_component<ModelComponent>()) {
                out << YAML::Key << "ModelComponent";
                out << YAML::BeginMap;

                auto& mc = entity.get_component<ModelComponent>();
                out << YAML::Key << "Filepath" << YAML::Value << mc.model->file_path;

                out << YAML::EndMap;
            }

            if(entity.has_component<DirectionalLightComponent>()) {
                out << YAML::Key << "DirectionalLightComponent";
                out << YAML::BeginMap;

                auto& dlc = entity.get_component<DirectionalLightComponent>();
                out << YAML::Key << "Color" << YAML::Value << dlc.color;
                out << YAML::Key << "Intensity" << YAML::Value << dlc.intensity;

                out << YAML::EndMap;
            }

            if(entity.has_component<PointLightComponent>()) {
                out << YAML::Key << "PointLightComponent";
                out << YAML::BeginMap;

                auto& plc = entity.get_component<PointLightComponent>();
                out << YAML::Key << "Color" << YAML::Value << plc.color;
                out << YAML::Key << "Intensity" << YAML::Value << plc.intensity;

                out << YAML::EndMap;
            }

            if(entity.has_component<SpotLightComponent>()) {
                out << YAML::Key << "SpotLightComponent";
                out << YAML::BeginMap;

                auto& slc = entity.get_component<SpotLightComponent>();
                out << YAML::Key << "Color" << YAML::Value << slc.color;
                out << YAML::Key << "Intensity" << YAML::Value << slc.intensity;
                out << YAML::Key << "CutOff" << YAML::Value << slc.cut_off;
                out << YAML::Key << "OuterCutOff" << YAML::Value << slc.outer_cut_off;

                out << YAML::EndMap;
            }

            out << YAML::EndMap;
		});

        out << YAML::EndSeq;
        out << YAML::EndMap;

        std::ofstream fout(path.data());
        fout << out.c_str();
    }

    void Scene::deserialize(const std::string_view& path) {
        YAML::Node data = YAML::LoadFile(path.data());
        if(!data["Scene"]) { throw std::runtime_error("scene corrupted"); }

        reset();
        this->name = data["Scene"].as<std::string>();

        auto entities = data["Entities"];
        if(!entities) { return; }

        for(auto entity : entities) {
            auto uuid = entity["Entity"].as<u64>();
            auto tag_component = entity["TagComponent"];

            Entity deserialized_entity = create_entity_with_UUID(tag_component["Tag"].as<std::string>(), uuid);

            auto transform_component = entity["TransformComponent"];
            if (transform_component) {
                auto &tc = deserialized_entity.add_component<TransformComponent>();
                tc.position = transform_component["Position"].as<glm::vec3>();
                tc.rotation = transform_component["Rotation"].as<glm::vec3>();
                tc.scale = transform_component["Scale"].as<glm::vec3>();
                tc.is_dirty = true;
            }

            auto camera_component = entity["CameraComponent"];
            if (camera_component) {
                auto &cc = deserialized_entity.add_component<CameraComponent>();
                cc.camera.fov = camera_component["FOV"].as<f32>();
                cc.camera.aspect = camera_component["Aspect"].as<f32>();
                cc.camera.near_clip = camera_component["NearPlane"].as<f32>();
                cc.camera.far_clip = camera_component["FarPlane"].as<f32>();
                cc.is_dirty = true;
            }

            auto model_component = entity["ModelComponent"];
            if (model_component) {
                auto& mc = deserialized_entity.add_component<ModelComponent>();
                mc.model = std::make_shared<Model>(device, model_component["Filepath"].as<std::string>());
            }

            auto directional_light_component = entity["DirectionalLightComponent"];
            if (directional_light_component) {
                auto& dlc = deserialized_entity.add_component<DirectionalLightComponent>();
                dlc.color = directional_light_component["Color"].as<glm::vec3>();
                dlc.intensity = directional_light_component["Intensity"].as<f32>();
            }

            auto point_light_component = entity["PointLightComponent"];
            if (point_light_component) {
                auto& plc = deserialized_entity.add_component<PointLightComponent>();
                plc.color = point_light_component["Color"].as<glm::vec3>();
                plc.intensity = point_light_component["Intensity"].as<f32>();
            }

            auto spot_light_component = entity["SpotLightComponent"];
            if (spot_light_component) {
                auto& slc = deserialized_entity.add_component<SpotLightComponent>();
                slc.color = point_light_component["Color"].as<glm::vec3>();
                slc.intensity = point_light_component["Intensity"].as<f32>();
                slc.cut_off = point_light_component["CutOff"].as<f32>();
                slc.outer_cut_off = point_light_component["OuterCutOff"].as<f32>();
            }
        }

        auto find_entt_handle = [&](u64 uuid) -> entt::entity {
            entt::entity handle = entt::null;

            registry->each([&](auto entity_handle){
                Entity entity = { entity_handle, this };
                if (!entity) { return; }

                if(entity.get_component<UUIDComponent>().uuid.uuid == uuid) { 
                    handle = entity.handle;
                }
            });

            if(handle == entt::null) { throw std::runtime_error("entity couldn't be found"); }
            return handle;
        };

        for(auto entity : entities) {
            auto uuid = entity["Entity"].as<u64>();

            Entity deserialized_entity = { find_entt_handle(uuid), this };

            auto relationship_component = entity["RelationshipComponent"];
            if (relationship_component) {
                auto& rc = deserialized_entity.get_component<RelationshipComponent>();

                u64 parent_uuid = relationship_component["Parent"].as<u64>();
                if(parent_uuid != 0) {
                    rc.parent = find_entt_handle(parent_uuid);
                }

                auto children = relationship_component["Children"];

                for(auto child : children) {
                    rc.children.push_back(find_entt_handle(child.as<u64>()));
                }
            }
        }
    }

    void Scene::reset() {
        // TODO: add reset function later flecs::world().reset()
        registry = std::make_unique<entt::registry>();
    }

    void Scene::update() {
        bool light_updated = false;

        iterate([&](Entity entity){
            if(entity.has_component<TransformComponent>()) {
                if(entity.get_component<TransformComponent>().is_dirty) {
                    if(entity.has_component<DirectionalLightComponent>() || entity.has_component<PointLightComponent>() || entity.has_component<SpotLightComponent>()) {
                        light_updated = true;
                    }
                }
            }

            if(entity.has_component<DirectionalLightComponent>()) {
                if(entity.get_component<DirectionalLightComponent>().is_dirty) {
                    light_updated = true;
                    entity.get_component<DirectionalLightComponent>().is_dirty = false;
                }
            }

            if(entity.has_component<PointLightComponent>()) {
                if(entity.get_component<PointLightComponent>().is_dirty) {
                    light_updated = true;
                    entity.get_component<PointLightComponent>().is_dirty = false;
                }
            }

            if(entity.has_component<SpotLightComponent>()) {
                if(entity.get_component<SpotLightComponent>().is_dirty) {
                    light_updated = true;
                    entity.get_component<SpotLightComponent>().is_dirty = false;
                }
            }
        });


        if(light_updated) {
            LightBuffer temp_light_buffer = {};
            temp_light_buffer.num_directional_lights = 0;
            temp_light_buffer.num_point_lights = 0;
            temp_light_buffer.num_spot_lights = 0;

            iterate([&](Entity entity){
                if(entity.has_component<TransformComponent>()) {
                    auto& tc = entity.get_component<TransformComponent>();

                    if(entity.has_component<DirectionalLightComponent>()) {
                        auto& light = entity.get_component<DirectionalLightComponent>();
                        auto& temp_light = temp_light_buffer.directional_lights[temp_light_buffer.num_directional_lights];

                        glm::vec3 rot = tc.rotation;
                        glm::vec3 dir = { 0.0f, 1.0f, 0.0f };
                        dir = glm::rotateX(dir, glm::radians(rot.x));
                        dir = glm::rotateY(dir, glm::radians(rot.y));
                        dir = glm::rotateZ(dir, glm::radians(rot.z));

                        temp_light.direction = *reinterpret_cast<const f32vec3 *>(&dir);
                        temp_light.color = *reinterpret_cast<const f32vec3 *>(&light.color);
                        temp_light.intensity = light.intensity;

                        temp_light_buffer.num_directional_lights++;
                    }

                    if(entity.has_component<PointLightComponent>()) {
                        auto& light = entity.get_component<PointLightComponent>();
                        auto& temp_light = temp_light_buffer.point_lights[temp_light_buffer.num_point_lights];

                        temp_light.position = *reinterpret_cast<const f32vec3 *>(&tc.position);
                        temp_light.color = *reinterpret_cast<const f32vec3 *>(&light.color);
                        temp_light.intensity = light.intensity;

                        temp_light_buffer.num_point_lights++;
                    }

                    if(entity.has_component<SpotLightComponent>()) {
                        auto& light = entity.get_component<SpotLightComponent>();
                        auto& temp_light = temp_light_buffer.spot_lights[temp_light_buffer.num_spot_lights];

                        glm::vec3 rot = tc.rotation;
                        glm::vec3 dir = { 0.0f, 1.0f, 0.0f };
                        dir = glm::rotateX(dir, glm::radians(rot.x));
                        dir = glm::rotateY(dir, glm::radians(rot.y));
                        dir = glm::rotateZ(dir, glm::radians(rot.z));

                        temp_light.position = *reinterpret_cast<const f32vec3 *>(&tc.position);
                        temp_light.direction = *reinterpret_cast<const f32vec3 *>(&dir);
                        temp_light.color = *reinterpret_cast<const f32vec3 *>(&light.color);
                        temp_light.intensity = light.intensity;
                        temp_light.cut_off = glm::cos(glm::radians(light.cut_off));
                        temp_light.outer_cut_off = glm::cos(glm::radians(light.outer_cut_off));

                        temp_light_buffer.num_spot_lights++;
                    }
                }
            });

            auto cmd_list = device.create_command_list({
                .debug_name = "",
            });

            daxa::BufferId staging_light_buffer = device.create_buffer({
                .memory_flags = daxa::MemoryFlagBits::HOST_ACCESS_RANDOM,
                .size = sizeof(LightBuffer),
                .debug_name = "light buffer",
            });

            cmd_list.destroy_buffer_deferred(staging_light_buffer);

            auto* buffer_ptr = device.get_host_address_as<LightBuffer>(staging_light_buffer);
            std::memcpy(buffer_ptr, &temp_light_buffer, sizeof(LightBuffer));

            cmd_list.copy_buffer_to_buffer({
                .src_buffer = staging_light_buffer,
                .dst_buffer = light_buffer,
                .size = sizeof(LightBuffer),
            });

            cmd_list.complete();
            device.submit_commands({
                .command_lists = {std::move(cmd_list)},
            });
        }

        iterate([&](Entity entity){
            entity.update(device);
        });
    }
}