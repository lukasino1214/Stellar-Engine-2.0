#pragma once

#include <entt/entt.hpp>

#include <data/components.hpp>
#include <core/types.hpp>
#include <physics/physics.hpp>

#include <functional>

#include <daxa/utils/pipeline_manager.hpp>

namespace Stellar {
    struct Entity;

    struct Scene {
        explicit Scene(const std::string_view& _name, daxa::Device _device, daxa::PipelineManager& pipeline_manager, const std::shared_ptr<Physics>& _physics);
        ~Scene();

        auto create_entity(const std::string_view& _name) -> Entity;
        auto create_entity_with_UUID(const std::string_view& _name, UUID _uuid) -> Entity;

        void destroy_entity(const Entity& entity);

        void iterate(std::function<void(Entity)> fn);

        void serialize(const std::string_view& path);
        void deserialize(const std::string_view& path);

        void reset();

        void update();

        std::string name;
        std::unique_ptr<entt::registry> registry;
        daxa::Device device;
        std::shared_ptr<Physics> physics;

        daxa::BufferId light_buffer;

        std::shared_ptr<daxa::RasterPipeline> normal_shadow_pipeline;
        std::shared_ptr<daxa::RasterPipeline> variance_shadow_pipeline;
        std::shared_ptr<daxa::RasterPipeline> filter_gauss_pipeline;

        daxa::SamplerId pcf_sampler;
    };
}