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
    Scene::Scene(const std::string_view& _name) : name{_name} {
        registry = std::make_unique<entt::registry>();
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

    void Scene::destroy_entity(Entity entity) {
        registry->destroy(entity.handle);
    }

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
}