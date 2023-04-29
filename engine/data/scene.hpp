#pragma once

#include <entt/entt.hpp>

//#include <data/components.hpp>
#include <core/uuid.hpp>
#include <core/types.hpp>
//#include <physics/physics.hpp>

#include <functional>

#include <daxa/utils/pipeline_manager.hpp>

namespace Stellar {
    struct Entity;
    struct Physics;

    struct Scene {
        explicit Scene(const std::string_view& _name, daxa::Device _device, daxa::PipelineManager& pipeline_manager);
        ~Scene();

        auto create_entity(const std::string_view& _name) -> Entity;
        auto create_entity_with_UUID(const std::string_view& _name, UUID _uuid) -> Entity;

        void destroy_entity(const Entity& entity);

        void iterate(std::function<void(Entity)> fn);

        void serialize(const std::string_view& path);
        void deserialize(const std::string_view& path);

        void reset();

        void update();

        void physics_update(f32 delta_time);

        std::string name;
        std::unique_ptr<entt::registry> registry;
        daxa::Device device;
        std::unique_ptr<Physics> physics;

        daxa::BufferId light_buffer;

        std::shared_ptr<daxa::RasterPipeline> normal_shadow_pipeline;
        std::shared_ptr<daxa::RasterPipeline> variance_shadow_pipeline;
        std::shared_ptr<daxa::RasterPipeline> filter_gauss_pipeline;

        daxa::SamplerId pcf_sampler;
    };
}