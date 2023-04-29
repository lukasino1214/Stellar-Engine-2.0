#pragma once

#include <core/types.hpp>
#include <core/uuid.hpp>
#include <entt/entt.hpp>
#include <graphics/camera.hpp>
//#include <graphics/model.hpp>
#include <daxa/daxa.hpp>
#include <physics/types.hpp>

namespace YAML {
    struct Emitter;
    struct Node;
};

namespace physx {
    struct PxMaterial;
    struct PxShape;
    struct PxActor;
};

//#include <yaml-cpp/yaml.h>

namespace Stellar {
    struct Entity;
    struct Model;

    struct UUIDComponent {
        UUID uuid = UUID();

        void draw();

        static void serialize(YAML::Emitter& out, Entity& entity);
        static void deserialize(YAML::Node& node, Entity& entity);
    };

    struct TagComponent {
        std::string name = "Empty Entity";

        void draw();

        static void serialize(YAML::Emitter& out, Entity& entity);
        static void deserialize(YAML::Node& node, Entity& entity);
    };

    struct RelationshipComponent {
        entt::entity parent = entt::null;
        std::vector<entt::entity> children = {};

        void draw();

        static void serialize(YAML::Emitter& out, Entity& entity);
        static void deserialize(YAML::Node& node, Entity& entity);
    };

    struct TransformComponent {
        glm::vec3 position = { 0.0f, 0.0f, 0.0f };
        glm::vec3 rotation{ 0.0001f, 0.0001f, 0.0001f };
        glm::vec3 scale = { 1.0f, 1.0f, 1.0f };
        glm::mat4 model_matrix{1.0f};
        glm::mat4 normal_matrix{1.0f};
        bool is_dirty = true;

        daxa::BufferId transform_buffer;

        void draw();

        static void serialize(YAML::Emitter& out, Entity& entity);
        static void deserialize(YAML::Node& node, Entity& entity);
    };

    struct CameraComponent {
        Camera3D camera;
        bool is_dirty = true;

        void draw();

        static void serialize(YAML::Emitter& out, Entity& entity);
        static void deserialize(YAML::Node& node, Entity& entity);
    };

    struct ModelComponent {
        std::string file_path = "";
        std::shared_ptr<Model> model;

        void draw(daxa::Device device);

        static void serialize(YAML::Emitter& out, Entity& entity);
        static void deserialize(YAML::Node& node, Entity& entity, daxa::Device& device);
    };

    struct ShadowInfo {
        glm::mat4 view{1.0f};
        glm::mat4 projection{1.0f};
        daxa::ImageId depth_image;
        daxa::ImageId shadow_image;
        daxa::ImageId temp_shadow_image;
        bool has_to_resize = false;
        glm::ivec2 image_size = {1024, 1024};
        f32 clip_space = 128.0f;
    };

    struct DirectionalLightComponent {
        glm::vec3 color = { 1.0f, 1.0f, 1.0f };
        f32 intensity = 1.0f;
        bool is_dirty = true;
        ShadowInfo shadow_info;

        void draw();

        static void serialize(YAML::Emitter& out, Entity& entity);
        static void deserialize(YAML::Node& node, Entity& entity);
    };

    struct PointLightComponent {
        glm::vec3 color = { 1.0f, 1.0f, 1.0f };
        f32 intensity = 32.0f;
        bool is_dirty = true;

        void draw();

        static void serialize(YAML::Emitter& out, Entity& entity);
        static void deserialize(YAML::Node& node, Entity& entity);
    };

    struct SpotLightComponent {
        glm::vec3 color = { 1.0f, 1.0f, 1.0f };
        f32 intensity = 4.0f;
        f32 cut_off = 20.0f;
        f32 outer_cut_off = 30.0f;
        bool is_dirty = true;
        ShadowInfo shadow_info;

        void draw();

        static void serialize(YAML::Emitter& out, Entity& entity);
        static void deserialize(YAML::Node& node, Entity& entity);
    };

    struct RigidBodyComponent {
        physx::PxMaterial* material = nullptr;
        physx::PxShape* shape = nullptr;
        physx::PxActor* body = nullptr;
        bool is_dirty = true;

        RigidBodyType rigid_body_type = RigidBodyType::Static;
        GeometryType geometry_type = GeometryType::Box;

        f32 radius = 0.5f;
        f32 half_height = 0.5f;
        glm::vec3 half_extent = { 0.5f, 0.5f, 0.5f };

        f32 density = 10.0f;
        f32 static_friction = 0.5f;
        f32 dynamic_friction = 0.5f;
        f32 restitution = 0.5f;

        void draw();

        static void serialize(YAML::Emitter& out, Entity& entity);
        static void deserialize(YAML::Node& node, Entity& entity);
    };
}