#include "core/uuid.hpp"
#include <data/components.hpp>
#include <data/scene.hpp>

#include <data/entity.hpp>
#include <entt/entity/entity.hpp>
#include <stdexcept>
#include <string>
#include <yaml-cpp/emittermanip.h>
#include <yaml-cpp/yaml.h>

#include <algorithm>

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

/*YAML::Emitter &operator<<(YAML::Emitter &out, const glm::vec3 &v) {
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
    return out;
}

YAML::Emitter &operator<<(YAML::Emitter &out, const glm::vec4 &v) {
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
    return out;
}*/

namespace Stellar {
    Scene::Scene(const std::string_view& _name, daxa::Device _device, daxa::PipelineManager& pipeline_manager, const std::shared_ptr<Physics>& _physics) : name{_name}, device{_device}, physics{_physics} {
        registry = std::make_unique<entt::registry>();

        light_buffer = device.create_buffer({
            .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
            .size = sizeof(LightBuffer),
            .debug_name = "light buffer",
        });

        normal_shadow_pipeline = pipeline_manager.add_raster_pipeline({
            .vertex_shader_info = {.source = daxa::ShaderFile{"normal_shadow.glsl"}, .compile_options = {.defines = {daxa::ShaderDefine{"DRAW_VERT"}}}},
            .fragment_shader_info = {.source = daxa::ShaderFile{"normal_shadow.glsl"}, .compile_options = {.defines = {daxa::ShaderDefine{"DRAW_FRAG"}}}},
            .color_attachments = {},
            .depth_test = {
                .depth_attachment_format = daxa::Format::D16_UNORM,
                .enable_depth_test = true,
                .enable_depth_write = true,
            },
            .raster = {
                .face_culling = daxa::FaceCullFlagBits::NONE,
            },
            .push_constant_size = sizeof(ShadowPush),
            .debug_name = "normal_shadow_pipeline",
        }).value();

        variance_shadow_pipeline = pipeline_manager.add_raster_pipeline({
            .vertex_shader_info = {.source = daxa::ShaderFile{"variance_shadow.glsl"}, .compile_options = {.defines = {daxa::ShaderDefine{"DRAW_VERT"}}}},
            .fragment_shader_info = {.source = daxa::ShaderFile{"variance_shadow.glsl"}, .compile_options = {.defines = {daxa::ShaderDefine{"DRAW_FRAG"}}}},
            .color_attachments = {
                { .format = daxa::Format::R16G16_UNORM },
            },
            .depth_test = {
                .depth_attachment_format = daxa::Format::D16_UNORM,
                .enable_depth_test = true,
                .enable_depth_write = true,
            },
            .raster = {
                .face_culling = daxa::FaceCullFlagBits::NONE,
            },
            .push_constant_size = sizeof(ShadowPush),
            .debug_name = "variance_shadow_pipeline",
        }).value();

        filter_gauss_pipeline = pipeline_manager.add_raster_pipeline({
            .vertex_shader_info = {.source = daxa::ShaderFile{"filter_gauss.glsl"}, .compile_options = {.defines = {daxa::ShaderDefine{"DRAW_VERT"}}}},
            .fragment_shader_info = {.source = daxa::ShaderFile{"filter_gauss.glsl"}, .compile_options = {.defines = {daxa::ShaderDefine{"DRAW_FRAG"}}}},
            .color_attachments = {
                { .format = daxa::Format::R16G16_UNORM },
            },
            .raster = {
                .face_culling = daxa::FaceCullFlagBits::NONE,
            },
            .push_constant_size = sizeof(GaussPush),
            .debug_name = "filter_gauss_pipeline",
        }).value();

        pcf_sampler = this->device.create_sampler({
            .magnification_filter = daxa::Filter::LINEAR,
            .minification_filter = daxa::Filter::LINEAR,
            .mipmap_filter = daxa::Filter::LINEAR,
            .address_mode_u = daxa::SamplerAddressMode::CLAMP_TO_BORDER,
            .address_mode_v = daxa::SamplerAddressMode::CLAMP_TO_BORDER,
            .address_mode_w = daxa::SamplerAddressMode::CLAMP_TO_BORDER,
            .mip_lod_bias = 0.0f,
            .enable_anisotropy = true,
            .max_anisotropy = 16.0f,
            .enable_compare = true,
            .compare_op = daxa::CompareOp::LESS,
            .min_lod = 0.0f,
            .max_lod = static_cast<f32>(1),
            .border_color = daxa::BorderColor::FLOAT_OPAQUE_WHITE,
            .enable_unnormalized_coordinates = false,
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

            if(entity.has_component<DirectionalLightComponent>()) {
                auto& lc = entity.get_component<DirectionalLightComponent>();
                if(!lc.shadow_info.shadow_image.is_empty()) {
                    device.destroy_image(lc.shadow_info.shadow_image);
                }
            }

            if(entity.has_component<SpotLightComponent>()) {
                auto& lc = entity.get_component<SpotLightComponent>();
                if(!lc.shadow_info.shadow_image.is_empty()) {
                    device.destroy_image(lc.shadow_info.shadow_image);
                }

                if(!lc.shadow_info.depth_image.is_empty()) {
                    device.destroy_image(lc.shadow_info.depth_image);
                }

                if(!lc.shadow_info.temp_shadow_image.is_empty()) {
                    device.destroy_image(lc.shadow_info.temp_shadow_image);
                }
            }
        });

        device.destroy_sampler(pcf_sampler);

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

        if(rc.parent != entt::null) {
            Entity parent{ rc.parent, this };
            auto& p_rc = parent.get_component<RelationshipComponent>();
            p_rc.children.erase(std::remove(p_rc.children.begin(), p_rc.children.end(), entity.handle), p_rc.children.end());
        }

        if(entity.has_component<TransformComponent>()) {
            device.destroy_buffer(entity.get_component<TransformComponent>().transform_buffer);
        }

        if(entity.has_component<RigidBodyComponent>()) {
            auto& pc = entity.get_component<RigidBodyComponent>();

            physics->gScene->removeActor(*pc.body);
            pc.shape->release();
            pc.material->release();
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
                TagComponent::serialize(out, entity);
            }

            if(entity.has_component<RelationshipComponent>()) {
                RelationshipComponent::serialize(out, entity);
            }

            if(entity.has_component<TransformComponent>()) {
                TransformComponent::serialize(out, entity);
            }

            if(entity.has_component<CameraComponent>()) {
                CameraComponent::serialize(out, entity);
            }

            if(entity.has_component<ModelComponent>()) {
                ModelComponent::serialize(out, entity);
            }

            if(entity.has_component<DirectionalLightComponent>()) {
                DirectionalLightComponent::serialize(out, entity);
            }

            if(entity.has_component<PointLightComponent>()) {
                PointLightComponent::serialize(out, entity);
            }

            if(entity.has_component<SpotLightComponent>()) {
                SpotLightComponent::serialize(out, entity);
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
                TransformComponent::deserialize(transform_component, deserialized_entity);
            }

            auto camera_component = entity["CameraComponent"];
            if (camera_component) {
                CameraComponent::deserialize(camera_component, deserialized_entity);
            }

            auto model_component = entity["ModelComponent"];
            if (model_component) {
                ModelComponent::deserialize(model_component, deserialized_entity, device);
            }

            auto directional_light_component = entity["DirectionalLightComponent"];
            if (directional_light_component) {
                DirectionalLightComponent::deserialize(directional_light_component, deserialized_entity);
            }

            auto point_light_component = entity["PointLightComponent"];
            if (point_light_component) {
                PointLightComponent::deserialize(point_light_component, deserialized_entity);
            }

            auto spot_light_component = entity["SpotLightComponent"];
            if (spot_light_component) {
                SpotLightComponent::deserialize(spot_light_component, deserialized_entity);
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
                        glm::vec3 dir = { 0.0f, -1.0f, 0.0f };
                        dir = glm::rotateX(dir, glm::radians(rot.x));
                        dir = glm::rotateY(dir, glm::radians(rot.y));
                        dir = glm::rotateZ(dir, glm::radians(rot.z));

                        f32 clip_space = light.shadow_info.clip_space;
                        light.shadow_info.projection = glm::ortho(-clip_space, clip_space, -clip_space, clip_space, -clip_space, clip_space);

                        glm::vec3 pos = entity.get_component<TransformComponent>().position;

                        glm::vec3 look_pos = pos + dir;
                        light.shadow_info.view = glm::lookAt(pos, look_pos, glm::vec3(0.0, -1.0, 0.0));

                        if(light.shadow_info.shadow_image.is_empty()) {
                            light.shadow_info.shadow_image = device.create_image({
                                .format = daxa::Format::D16_UNORM,
                                .aspect = daxa::ImageAspectFlagBits::DEPTH,
                                .size = {static_cast<u32>(light.shadow_info.image_size.x), static_cast<u32>(light.shadow_info.image_size.y), 1},
                                .usage = daxa::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT | daxa::ImageUsageFlagBits::SHADER_READ_ONLY,
                            });
                        }

                        temp_light.direction = *reinterpret_cast<const f32vec3 *>(&dir);
                        temp_light.color = *reinterpret_cast<const f32vec3 *>(&light.color);
                        temp_light.intensity = light.intensity;

                        temp_light.shadow_image = TextureId { .texture_id = light.shadow_info.shadow_image.default_view(), .sampler_id = pcf_sampler };

                        glm::mat4 light_matrix = light.shadow_info.projection * light.shadow_info.view;

                        temp_light.light_matrix = *reinterpret_cast<const f32mat4x4*>(&light_matrix);

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
                        glm::vec3 dir = { 0.0f, -1.0f, 0.0f };
                        dir = glm::rotateX(dir, glm::radians(rot.x));
                        dir = glm::rotateY(dir, glm::radians(rot.y));
                        dir = glm::rotateZ(dir, glm::radians(rot.z));

                        f32 clip_space = light.shadow_info.clip_space;

                        light.shadow_info.projection = glm::perspective(glm::radians(light.outer_cut_off), 1.0f, 0.1f, clip_space);
                        light.shadow_info.projection[1][1] *= -1.0f;

                        glm::vec3 pos = entity.get_component<TransformComponent>().position;

                        glm::vec3 look_pos = pos + dir;
                        light.shadow_info.view = glm::lookAt(pos, look_pos, glm::vec3(0.0, 1.0, 0.0));

                        if(light.shadow_info.shadow_image.is_empty()) {
                            light.shadow_info.depth_image = device.create_image({
                                .format = daxa::Format::D16_UNORM,
                                .aspect = daxa::ImageAspectFlagBits::DEPTH,
                                .size = {static_cast<u32>(light.shadow_info.image_size.x), static_cast<u32>(light.shadow_info.image_size.y), 1},
                                .usage = daxa::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT | daxa::ImageUsageFlagBits::SHADER_READ_ONLY,
                            });

                            light.shadow_info.shadow_image = device.create_image({
                                .format = daxa::Format::R16G16_UNORM,
                                .aspect = daxa::ImageAspectFlagBits::COLOR,
                                .size = {static_cast<u32>(light.shadow_info.image_size.x), static_cast<u32>(light.shadow_info.image_size.y), 1},
                                .usage = daxa::ImageUsageFlagBits::COLOR_ATTACHMENT | daxa::ImageUsageFlagBits::SHADER_READ_ONLY,
                            });

                            light.shadow_info.temp_shadow_image = device.create_image({
                                .format = daxa::Format::R16G16_UNORM,
                                .aspect = daxa::ImageAspectFlagBits::COLOR,
                                .size = {static_cast<u32>(light.shadow_info.image_size.x), static_cast<u32>(light.shadow_info.image_size.y), 1},
                                .usage = daxa::ImageUsageFlagBits::COLOR_ATTACHMENT | daxa::ImageUsageFlagBits::SHADER_READ_ONLY,
                            });
                        }

                        temp_light.position = *reinterpret_cast<const f32vec3 *>(&tc.position);
                        temp_light.direction = *reinterpret_cast<const f32vec3 *>(&dir);
                        temp_light.color = *reinterpret_cast<const f32vec3 *>(&light.color);
                        temp_light.intensity = light.intensity;
                        temp_light.cut_off = glm::cos(glm::radians(light.cut_off));
                        temp_light.outer_cut_off = glm::cos(glm::radians(light.outer_cut_off));

                        temp_light.shadow_image = TextureId { .texture_id = light.shadow_info.shadow_image.default_view(), .sampler_id = pcf_sampler };

                        glm::mat4 light_matrix = light.shadow_info.projection * light.shadow_info.view;

                        temp_light.light_matrix = *reinterpret_cast<const f32mat4x4*>(&light_matrix);

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

            /*std::cout << "here" << std::endl;

            iterate([&](Entity light_entity){
                if(light_entity.has_component<DirectionalLightComponent>()) {
                    std::cout << "here 1" << std::endl;
                    auto& light = light_entity.get_component<DirectionalLightComponent>();

                    u32 size_x = static_cast<u32>(light.shadow_info.image_size.x);
                    u32 size_y = static_cast<u32>(light.shadow_info.image_size.y);
                    
                    cmd_list.pipeline_barrier_image_transition({
                        .waiting_pipeline_access = daxa::AccessConsts::TRANSFER_WRITE,
                        .before_layout = daxa::ImageLayout::UNDEFINED,
                        .after_layout = daxa::ImageLayout::GENERAL,
                        .image_slice = {.image_aspect = daxa::ImageAspectFlagBits::DEPTH },
                        .image_id = light.shadow_info.shadow_image,
                    });

                    cmd_list.begin_renderpass({
                        .depth_attachment = {{
                            .image_view = light.shadow_info.shadow_image.default_view(),
                            .load_op = daxa::AttachmentLoadOp::CLEAR,
                            .clear_value = daxa::DepthValue{1.0f, 0},
                        }},
                        .render_area = {.x = 0, .y = 0, .width = size_x, .height = size_y},
                    });
                    cmd_list.set_pipeline(*normal_shadow_pipeline);

                    ShadowPush push;
                    glm::mat4 vp = light.shadow_info.projection * light.shadow_info.view;
                    push.light_matrix = *reinterpret_cast<const f32mat4x4*>(&vp);

                    iterate([&](Entity entity){
                        if(entity.has_component<ModelComponent>()) {
                            auto& model = entity.get_component<ModelComponent>().model;
                            auto& tc = entity.get_component<TransformComponent>();

                            push.transform_buffer = device.get_device_address(tc.transform_buffer);

                            model->draw(cmd_list, push);
                        }
                    });

                    //cmd_list.end_renderpass();

                }

                if(light_entity.has_component<SpotLightComponent>()) {
                    std::cout << "here 2" << std::endl;
                    auto& light = light_entity.get_component<SpotLightComponent>();

                    u32 size_x = static_cast<u32>(light.shadow_info.image_size.x);
                    u32 size_y = static_cast<u32>(light.shadow_info.image_size.y);
                    
                    cmd_list.pipeline_barrier_image_transition({
                        .waiting_pipeline_access = daxa::AccessConsts::TRANSFER_WRITE,
                        .before_layout = daxa::ImageLayout::UNDEFINED,
                        .after_layout = daxa::ImageLayout::GENERAL,
                        .image_slice = {.image_aspect = daxa::ImageAspectFlagBits::DEPTH },
                        .image_id = light.shadow_info.depth_image,
                    });

                    cmd_list.pipeline_barrier_image_transition({
                        .waiting_pipeline_access = daxa::AccessConsts::TRANSFER_WRITE,
                        .before_layout = daxa::ImageLayout::UNDEFINED,
                        .after_layout = daxa::ImageLayout::GENERAL,
                        .image_slice = {.image_aspect = daxa::ImageAspectFlagBits::COLOR },
                        .image_id = light.shadow_info.shadow_image,
                    });

                    cmd_list.pipeline_barrier_image_transition({
                        .waiting_pipeline_access = daxa::AccessConsts::TRANSFER_WRITE,
                        .before_layout = daxa::ImageLayout::UNDEFINED,
                        .after_layout = daxa::ImageLayout::GENERAL,
                        .image_slice = {.image_aspect = daxa::ImageAspectFlagBits::COLOR },
                        .image_id = light.shadow_info.temp_shadow_image,
                    });

                    cmd_list.begin_renderpass({
                        .color_attachments = {
                            {
                                .image_view = light.shadow_info.shadow_image.default_view(),
                                .load_op = daxa::AttachmentLoadOp::CLEAR,
                                .clear_value = std::array<f32, 4>{1.0f, 1.0f, 1.0f, 1.0f},
                            },
                        },
                        .depth_attachment = {{
                            .image_view = light.shadow_info.depth_image.default_view(),
                            .load_op = daxa::AttachmentLoadOp::CLEAR,
                            .clear_value = daxa::DepthValue{1.0f, 0},
                        }},
                        .render_area = {.x = 0, .y = 0, .width = size_x, .height = size_y},
                    });
                    cmd_list.set_pipeline(*variance_shadow_pipeline);

                    ShadowPush push;
                    glm::mat4 vp = light.shadow_info.projection * light.shadow_info.view;
                    push.light_matrix = *reinterpret_cast<const f32mat4x4*>(&vp);

                    iterate([&](Entity entity){
                        if(entity.has_component<ModelComponent>()) {
                            auto& model = entity.get_component<ModelComponent>().model;
                            auto& tc = entity.get_component<TransformComponent>();
                            if(tc.transform_buffer.is_empty()) {
                                return;
                            }

                            push.transform_buffer = device.get_device_address(tc.transform_buffer);

                            model->draw(cmd_list, push);
                        }
                    });

                    cmd_list.end_renderpass();

                    cmd_list.begin_renderpass({
                        .color_attachments = {
                            {
                                .image_view = light.shadow_info.temp_shadow_image.default_view(),
                                .load_op = daxa::AttachmentLoadOp::CLEAR,
                                .clear_value = std::array<f32, 4>{1.0f, 1.0f, 1.0f, 1.0f},
                            },
                        },
                        .render_area = {.x = 0, .y = 0, .width = size_x, .height = size_y},
                    });
                    cmd_list.set_pipeline(*filter_gauss_pipeline);

                    
                    cmd_list.push_constant(GaussPush {
                        .src_texture = light.shadow_info.shadow_image.default_view(),
                        .blur_scale = { 1.0f / static_cast<f32>(size_x), 0.0f }
                    });
                    cmd_list.draw({ .vertex_count = 3});

                    cmd_list.end_renderpass();

                    cmd_list.begin_renderpass({
                        .color_attachments = {
                            {
                                .image_view = light.shadow_info.shadow_image.default_view(),
                                .load_op = daxa::AttachmentLoadOp::CLEAR,
                                .clear_value = std::array<f32, 4>{1.0f, 1.0f, 1.0f, 1.0f},
                            },
                        },
                        .render_area = {.x = 0, .y = 0, .width = size_x, .height = size_y},
                    });
                    cmd_list.set_pipeline(*filter_gauss_pipeline);

                    
                    cmd_list.push_constant(GaussPush {
                        .src_texture = light.shadow_info.temp_shadow_image.default_view(),
                        .blur_scale = { 0.0f, 1.0f / static_cast<f32>(size_y) }
                    });
                    cmd_list.draw({ .vertex_count = 3});

                    cmd_list.end_renderpass();
                }
            });*/

            cmd_list.complete();
            device.submit_commands({
                .command_lists = {std::move(cmd_list)},
            });
        }

        iterate([&](Entity entity){
            entity.update(device, physics);
        });

        auto cmd_list = device.create_command_list({
            .debug_name = "",
        });


        iterate([&](Entity light_entity){
            if(light_entity.has_component<DirectionalLightComponent>()) {
                auto& light = light_entity.get_component<DirectionalLightComponent>();

                u32 size_x = static_cast<u32>(light.shadow_info.image_size.x);
                u32 size_y = static_cast<u32>(light.shadow_info.image_size.y);
                
                cmd_list.pipeline_barrier_image_transition({
                    .waiting_pipeline_access = daxa::AccessConsts::TRANSFER_WRITE,
                    .before_layout = daxa::ImageLayout::UNDEFINED,
                    .after_layout = daxa::ImageLayout::GENERAL,
                    .image_slice = {.image_aspect = daxa::ImageAspectFlagBits::DEPTH },
                    .image_id = light.shadow_info.shadow_image,
                });

                cmd_list.begin_renderpass({
                    .depth_attachment = {{
                        .image_view = light.shadow_info.shadow_image.default_view(),
                        .load_op = daxa::AttachmentLoadOp::CLEAR,
                        .clear_value = daxa::DepthValue{1.0f, 0},
                    }},
                    .render_area = {.x = 0, .y = 0, .width = size_x, .height = size_y},
                });
                cmd_list.set_pipeline(*normal_shadow_pipeline);

                ShadowPush push;
                glm::mat4 vp = light.shadow_info.projection * light.shadow_info.view;
                push.light_matrix = *reinterpret_cast<const f32mat4x4*>(&vp);

                iterate([&](Entity entity){
                    if(entity.has_component<ModelComponent>()) {
                        auto& model = entity.get_component<ModelComponent>().model;
                        if(!model) { return; }
                        auto& tc = entity.get_component<TransformComponent>();

                        push.transform_buffer = device.get_device_address(tc.transform_buffer);

                        model->draw(cmd_list, push);
                    }
                });

                cmd_list.end_renderpass();

            }

            if(light_entity.has_component<SpotLightComponent>()) {
                auto& light = light_entity.get_component<SpotLightComponent>();

                u32 size_x = static_cast<u32>(light.shadow_info.image_size.x);
                u32 size_y = static_cast<u32>(light.shadow_info.image_size.y);
                
                cmd_list.pipeline_barrier_image_transition({
                    .waiting_pipeline_access = daxa::AccessConsts::TRANSFER_WRITE,
                    .before_layout = daxa::ImageLayout::UNDEFINED,
                    .after_layout = daxa::ImageLayout::GENERAL,
                    .image_slice = {.image_aspect = daxa::ImageAspectFlagBits::DEPTH },
                    .image_id = light.shadow_info.depth_image,
                });

                cmd_list.pipeline_barrier_image_transition({
                    .waiting_pipeline_access = daxa::AccessConsts::TRANSFER_WRITE,
                    .before_layout = daxa::ImageLayout::UNDEFINED,
                    .after_layout = daxa::ImageLayout::GENERAL,
                    .image_slice = {.image_aspect = daxa::ImageAspectFlagBits::COLOR },
                    .image_id = light.shadow_info.shadow_image,
                });

                cmd_list.pipeline_barrier_image_transition({
                    .waiting_pipeline_access = daxa::AccessConsts::TRANSFER_WRITE,
                    .before_layout = daxa::ImageLayout::UNDEFINED,
                    .after_layout = daxa::ImageLayout::GENERAL,
                    .image_slice = {.image_aspect = daxa::ImageAspectFlagBits::COLOR },
                    .image_id = light.shadow_info.temp_shadow_image,
                });

                cmd_list.begin_renderpass({
                    .color_attachments = {
                        {
                            .image_view = light.shadow_info.shadow_image.default_view(),
                            .load_op = daxa::AttachmentLoadOp::CLEAR,
                            .clear_value = std::array<f32, 4>{1.0f, 1.0f, 1.0f, 1.0f},
                        },
                    },
                    .depth_attachment = {{
                        .image_view = light.shadow_info.depth_image.default_view(),
                        .load_op = daxa::AttachmentLoadOp::CLEAR,
                        .clear_value = daxa::DepthValue{1.0f, 0},
                    }},
                    .render_area = {.x = 0, .y = 0, .width = size_x, .height = size_y},
                });
                cmd_list.set_pipeline(*variance_shadow_pipeline);

                ShadowPush push;
                glm::mat4 vp = light.shadow_info.projection * light.shadow_info.view;
                push.light_matrix = *reinterpret_cast<const f32mat4x4*>(&vp);

                iterate([&](Entity entity){
                    if(entity.has_component<ModelComponent>()) {
                        auto& model = entity.get_component<ModelComponent>().model;
                        if(!model) { return; }
                        auto& tc = entity.get_component<TransformComponent>();

                        push.transform_buffer = device.get_device_address(tc.transform_buffer);

                        model->draw(cmd_list, push);
                    }
                });

                cmd_list.end_renderpass();

                cmd_list.begin_renderpass({
                    .color_attachments = {
                        {
                            .image_view = light.shadow_info.temp_shadow_image.default_view(),
                            .load_op = daxa::AttachmentLoadOp::CLEAR,
                            .clear_value = std::array<f32, 4>{1.0f, 1.0f, 1.0f, 1.0f},
                        },
                    },
                    .render_area = {.x = 0, .y = 0, .width = size_x, .height = size_y},
                });
                cmd_list.set_pipeline(*filter_gauss_pipeline);

                
                cmd_list.push_constant(GaussPush {
                    .src_texture = light.shadow_info.shadow_image.default_view(),
                    .blur_scale = { 1.0f / static_cast<f32>(size_x), 0.0f }
                });
                cmd_list.draw({ .vertex_count = 3});

                cmd_list.end_renderpass();

                cmd_list.begin_renderpass({
                    .color_attachments = {
                        {
                            .image_view = light.shadow_info.shadow_image.default_view(),
                            .load_op = daxa::AttachmentLoadOp::CLEAR,
                            .clear_value = std::array<f32, 4>{1.0f, 1.0f, 1.0f, 1.0f},
                        },
                    },
                    .render_area = {.x = 0, .y = 0, .width = size_x, .height = size_y},
                });
                cmd_list.set_pipeline(*filter_gauss_pipeline);

                
                cmd_list.push_constant(GaussPush {
                    .src_texture = light.shadow_info.temp_shadow_image.default_view(),
                    .blur_scale = { 0.0f, 1.0f / static_cast<f32>(size_y) }
                });
                cmd_list.draw({ .vertex_count = 3});

                cmd_list.end_renderpass();
            }
        });

        cmd_list.complete();
        device.submit_commands({
            .command_lists = {std::move(cmd_list)},
        });
    }
}