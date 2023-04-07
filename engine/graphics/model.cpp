#include <assimp/material.h>
#include <assimp/texture.h>
#include <functional>
#include <graphics/model.hpp>

#include <filesystem>
#include <stdexcept>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <string>

namespace Stellar {
    Model::Model(daxa::Device _device, const std::string_view& file_path) : device{_device}  {
        if(!std::filesystem::exists(file_path)) {
            throw std::runtime_error("couldn't find a model");
        }

        constexpr static u32 process_flags = aiProcess_Triangulate | aiProcess_OptimizeMeshes | aiProcess_JoinIdenticalVertices |
			            aiProcess_GenNormals | aiProcess_CalcTangentSpace | aiProcess_PreTransformVertices | aiProcess_FlipUVs;
			//aiProcess_MakeLeftHanded |

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(file_path.data(), process_flags);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            throw std::runtime_error("Error importing file" + std::string{file_path} + importer.GetErrorString());
		}

        auto search_textures = [&](const std::string& path) -> u32 {
            for(u32 i = 0; i < textures.size(); i++) {
                if(textures[i]->path == path) {
                    return i;
                }
            }

            std::unique_ptr<Texture> texture = std::make_unique<Texture>(device, path);
            textures.push_back(std::move(texture));
            return textures.size() - 1;
        };

        std::string format = std::string{file_path.substr(file_path.find_last_of(".") + 1)};
        
        for(usize i = 0; i < scene->mNumMaterials; i++) {
            const aiMaterial* material = scene->mMaterials[i];

            Material my_material;

            if (aiString diffuseTex{}; aiGetMaterialTexture(material, aiTextureType_DIFFUSE, 0, &diffuseTex) == AI_SUCCESS) {
				std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + diffuseTex.C_Str();
                my_material.albedo_texture_index = search_textures(texture);
			}

            materials.push_back(my_material);
        }

        struct Vertex {
            glm::vec3 positions = { 0.0f, 0.0f, 0.0f};
            glm::vec2 uvs = { 0.0f, 0.0f };
            glm::vec3 normals = { 0.0f, 0.0f, 0.0f};
            glm::vec3 tangets = { 0.0f, 0.0f, 0.0f};
        };

        std::vector<Vertex> vertices{};
        std::vector<u32> indices{};

        uint32_t vertexOffset = 0;
        uint32_t indexOffset = 0;


        auto load_submesh = [&](aiMesh* mesh) -> void {
            f32 factor{ 1.0f };
            const u32 index = mesh->mMaterialIndex;
            const aiMaterial* material = scene->mMaterials[index];
            std::string materialSlot = material->GetName().C_Str();
            if (format == "fbx") { factor *= 0.01f; }
            if (format == "gltf") { materialSlot = "material_" + std::to_string(index); }

            uint32_t vertexCount = 0;
            uint32_t indexCount = 0;

            for (u32 i = 0; i < mesh->mNumVertices; i++) {
                vertices.push_back(Vertex {
                    .positions = {
                        factor * mesh->mVertices[i].x,
                        factor * mesh->mVertices[i].y,
                        factor * mesh->mVertices[i].z
                    },
                    .uvs = {
                        mesh->mTextureCoords[0][i].x,
                        mesh->mTextureCoords[0][i].y
                    },
                    .normals = {
                        mesh->mNormals[i].x,
                        mesh->mNormals[i].y,
                        mesh->mNormals[i].z
                    },
                    .tangets = {
                        mesh->mNormals[i].x,
                        mesh->mNormals[i].y,
                        mesh->mNormals[i].z
                    }
                });
                vertexCount++;
            }

            for (u32 i = 0; i < mesh->mNumFaces; i++) {
                aiFace face = mesh->mFaces[i];
                for (u32 j = 0; j < face.mNumIndices; j++) {
                    indices.push_back(face.mIndices[j]);
                    indexCount++;
                }
            }

             Primitive temp_primitive {
                .first_index = indexOffset,
                .first_vertex = vertexOffset,
                .index_count = indexCount,
                .vertex_count = vertexCount,
                .material_index = index
            };

            primitives.push_back(temp_primitive);

            vertexOffset += vertexCount;
            indexOffset += indexCount;
        };

        std::function<void(aiNode* node)> process_node;
        process_node = [&](aiNode* node) -> void {
            for (usize i = 0; i < node->mNumMeshes; i++) {
                aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
                load_submesh(mesh);
            }

            for (usize i = 0; i < node->mNumChildren; i++) {
                process_node(node->mChildren[i]);
            }
        };

        process_node(scene->mRootNode);

        face_buffer = device.create_buffer(daxa::BufferInfo{
            .size = static_cast<u32>(sizeof(Vertex) * vertices.size()),
            .debug_name = "",
        });

        index_buffer = device.create_buffer(daxa::BufferInfo{
            .size = static_cast<u32>(sizeof(u32) * indices.size()),
            .debug_name = "",
        });

        {
            auto cmd_list = device.create_command_list({
                .debug_name = "",
            });

            auto vertex_staging_buffer = device.create_buffer({
                .memory_flags = daxa::MemoryFlagBits::HOST_ACCESS_RANDOM,
                .size = static_cast<u32>(sizeof(Vertex) * vertices.size()),
                .debug_name = "",
            });
            cmd_list.destroy_buffer_deferred(vertex_staging_buffer);

            auto buffer_ptr = device.get_host_address_as<Vertex>(vertex_staging_buffer);
            std::memcpy(buffer_ptr, vertices.data(), vertices.size() * sizeof(Vertex));

            cmd_list.pipeline_barrier({
                .awaited_pipeline_access = daxa::AccessConsts::HOST_WRITE,
                .waiting_pipeline_access = daxa::AccessConsts::TRANSFER_READ,
            });

            cmd_list.copy_buffer_to_buffer({
                .src_buffer = vertex_staging_buffer,
                .dst_buffer = face_buffer,
                .size = static_cast<u32>(sizeof(Vertex) * vertices.size()),
            });

            cmd_list.pipeline_barrier({
                .awaited_pipeline_access = daxa::AccessConsts::TRANSFER_WRITE,
                .waiting_pipeline_access = daxa::AccessConsts::VERTEX_SHADER_READ,
            });
            cmd_list.complete();
            device.submit_commands({
                .command_lists = {std::move(cmd_list)},
            });
        }

        {
            auto cmd_list = device.create_command_list({
                .debug_name = "",
            });

            auto index_staging_buffer = device.create_buffer({
                .memory_flags = daxa::MemoryFlagBits::HOST_ACCESS_RANDOM,
                .size = static_cast<u32>(sizeof(u32) * indices.size()),
                .debug_name = "",
            });
            cmd_list.destroy_buffer_deferred(index_staging_buffer);

            auto* buffer_ptr = device.get_host_address_as<u32>(index_staging_buffer);
            std::memcpy(buffer_ptr, indices.data(), indices.size() * sizeof(u32));

            cmd_list.pipeline_barrier({
                .awaited_pipeline_access = daxa::AccessConsts::HOST_WRITE,
                .waiting_pipeline_access = daxa::AccessConsts::TRANSFER_READ,
            });

            cmd_list.copy_buffer_to_buffer({
                .src_buffer = index_staging_buffer,
                .dst_buffer = index_buffer,
                .size = static_cast<u32>(sizeof(u32) * indices.size()),
            });

            cmd_list.pipeline_barrier({
                .awaited_pipeline_access = daxa::AccessConsts::TRANSFER_WRITE,
                .waiting_pipeline_access = daxa::AccessConsts::VERTEX_SHADER_READ,
            });
            cmd_list.complete();
            device.submit_commands({
                .command_lists = {std::move(cmd_list)},
            });
        }
    }

    Model::~Model() {
        device.destroy_buffer(face_buffer);
        device.destroy_buffer(index_buffer);
    }

    void Model::draw(daxa::CommandList& cmd_list, DepthPrepassPush& draw_push) {
        for (auto & primitive : primitives) {
            draw_push.vertex_buffer = device.get_device_address(face_buffer);
            cmd_list.push_constant(draw_push);

            if (primitive.index_count > 0) {
                cmd_list.set_index_buffer(index_buffer, 0, 4);
                cmd_list.draw_indexed({
                    .index_count = primitive.index_count,
                    .instance_count = 1,
                    .first_index = primitive.first_index,
                    .vertex_offset = static_cast<i32>(primitive.first_vertex),
                    .first_instance = 0,
                });
            } else {
                cmd_list.draw({
                    .vertex_count = primitive.vertex_count,
                    .instance_count = 1,
                    .first_vertex = primitive.first_vertex,
                    .first_instance = 0
                });
            }
        }
    }

    void Model::draw(daxa::CommandList& cmd_list, DrawPush& draw_push) {
        for (auto & primitive : primitives) {
            draw_push.vertex_buffer = device.get_device_address(face_buffer);
            auto& texture = textures[materials[primitive.material_index].albedo_texture_index];
            draw_push.texture = texture->image_id.default_view();
            draw_push.texture_sampler = texture->sampler_id;
            cmd_list.push_constant(draw_push);

            if (primitive.index_count > 0) {
                cmd_list.set_index_buffer(index_buffer, 0, 4);
                cmd_list.draw_indexed({
                    .index_count = primitive.index_count,
                    .instance_count = 1,
                    .first_index = primitive.first_index,
                    .vertex_offset = static_cast<i32>(primitive.first_vertex),
                    .first_instance = 0,
                });
            } else {
                cmd_list.draw({
                    .vertex_count = primitive.vertex_count,
                    .instance_count = 1,
                    .first_vertex = primitive.first_vertex,
                    .first_instance = 0
                });
            }
        }
    }
}