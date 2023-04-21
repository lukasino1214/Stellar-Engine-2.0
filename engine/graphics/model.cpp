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

#include <utils/threadpool.hpp>

namespace Stellar {
    Model::Model(daxa::Device _device, const std::string_view& file_path) : device{_device} {
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

        auto search_textures = [&](const std::string& path, daxa::Format format) -> u32 {
            for(u32 i = 0; i < textures.size(); i++) {
                if(textures[i]->path == path) {
                    return i;
                }
            }

            throw std::runtime_error("ooops");
            return 0;
            // Uncomment this when debugging some fuckery
            /*std::unique_ptr<Texture> texture = std::make_unique<Texture>(device, path, format);
            textures.push_back(std::move(texture));
            return textures.size() - 1;*/
        };

        std::string format = std::string{file_path.substr(file_path.find_last_of(".") + 1)};

        struct TextureHelper {
            std::string path;
            daxa::Format format;
        };

        std::vector<TextureHelper> texture_helpers = {};
        auto vec_set = [&](const std::string& path, daxa::Format _format){
            for(auto& tex : texture_helpers) {
                if(tex.path == path) { return; }
            }

            texture_helpers.push_back({
                .path = path,
                .format = _format
            });
        };

        for(usize i = 0; i < scene->mNumMaterials; i++) {
            const aiMaterial* material = scene->mMaterials[i];

            if (aiString diffuseTex{}; aiGetMaterialTexture(material, aiTextureType_DIFFUSE, 0, &diffuseTex) == AI_SUCCESS) {
				std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + diffuseTex.C_Str();
                vec_set(texture, daxa::Format::R8G8B8A8_SRGB);
			}

            if (aiString specularTex{}; aiGetMaterialTexture(material, aiTextureType_SPECULAR, 0, &specularTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + specularTex.C_Str();
                //std::cout << "aiTextureType_SPECULAR: " << texture << std::endl;
                throw std::runtime_error("unhandled aiTextureType_SPECULAR: " + texture);
			}

            if (aiString ambientTex{}; aiGetMaterialTexture(material, aiTextureType_AMBIENT, 0, &ambientTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + ambientTex.C_Str();
                throw std::runtime_error("unhandled aiTextureType_AMBIENT: " + texture);
			}

            if (aiString emissiveTex{}; aiGetMaterialTexture(material, aiTextureType_EMISSIVE, 0, &emissiveTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + emissiveTex.C_Str();
                throw std::runtime_error("unhandled aiTextureType_EMISSIVE: " + texture);
			}

            if (aiString heightTex{}; aiGetMaterialTexture(material, aiTextureType_HEIGHT, 0, &heightTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + heightTex.C_Str();
                throw std::runtime_error("unhandled aiTextureType_HEIGHT: " + texture);
			}

            if (aiString normalTex{}; aiGetMaterialTexture(material, aiTextureType_NORMALS, 0, &normalTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + normalTex.C_Str();
                vec_set(texture, daxa::Format::R8G8B8A8_UNORM);
			}

            if (aiString shininessTex{}; aiGetMaterialTexture(material, aiTextureType_SHININESS, 0, &shininessTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + shininessTex.C_Str();
                throw std::runtime_error("unhandled aiTextureType_SHININESS: " + texture);
			}

            if (aiString opacityTex{}; aiGetMaterialTexture(material, aiTextureType_OPACITY, 0, &opacityTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + opacityTex.C_Str();
                throw std::runtime_error("unhandled aiTextureType_OPACITY: " + texture);
			}

            if (aiString displacementTex{}; aiGetMaterialTexture(material, aiTextureType_DISPLACEMENT, 0, &displacementTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + displacementTex.C_Str();
                throw std::runtime_error("unhandled aiTextureType_DISPLACEMENT: " + texture);
			}

            if (aiString lightmapTex{}; aiGetMaterialTexture(material, aiTextureType_LIGHTMAP, 0, &lightmapTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + lightmapTex.C_Str();
                //throw std::runtime_error("unhandled aiTextureType_LIGHTMAP: " + texture);
			}

            if (aiString reflectionTex{}; aiGetMaterialTexture(material, aiTextureType_REFLECTION, 0, &reflectionTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + reflectionTex.C_Str();
                throw std::runtime_error("unhandled aiTextureType_REFLECTION: " + texture);
			}

            if (aiString baseColorTex{}; aiGetMaterialTexture(material, aiTextureType_BASE_COLOR, 0, &baseColorTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + baseColorTex.C_Str();
			}

            if (aiString normalCameraTex{}; aiGetMaterialTexture(material, aiTextureType_NORMAL_CAMERA, 0, &normalCameraTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + normalCameraTex.C_Str();
                throw std::runtime_error("unhandled aiTextureType_NORMAL_CAMERA: " + texture);
			}

            if (aiString emissionColorTex{}; aiGetMaterialTexture(material, aiTextureType_EMISSION_COLOR, 0, &emissionColorTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + emissionColorTex.C_Str();
                throw std::runtime_error("unhandled aiTextureType_EMISSION_COLOR: " + texture);
			}

            if (aiString metalnessTex{}; aiGetMaterialTexture(material, aiTextureType_METALNESS, 0, &metalnessTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + metalnessTex.C_Str();
                vec_set(texture, daxa::Format::R8G8B8A8_UNORM);
			}

            if (aiString roughnessTex{}; aiGetMaterialTexture(material, aiTextureType_DIFFUSE_ROUGHNESS, 0, &roughnessTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + roughnessTex.C_Str();
                vec_set(texture, daxa::Format::R8G8B8A8_UNORM);
			}

            if (aiString ambientOcclusionTex{}; aiGetMaterialTexture(material, aiTextureType_AMBIENT_OCCLUSION, 0, &ambientOcclusionTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + ambientOcclusionTex.C_Str();
                throw std::runtime_error("unhandled aiTextureType_AMBIENT_OCCLUSION: " + texture);
			}

            if (aiString sheenTex{}; aiGetMaterialTexture(material, aiTextureType_SHEEN, 0, &sheenTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + sheenTex.C_Str();
                throw std::runtime_error("unhandled aiTextureType_SHEEN: " + texture);
			}

            if (aiString clearcoatTex{}; aiGetMaterialTexture(material, aiTextureType_CLEARCOAT, 0, &clearcoatTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + clearcoatTex.C_Str();
                throw std::runtime_error("unhandled aiTextureType_CLEARCOAT: " + texture);
			}

            if (aiString transmissionTex{}; aiGetMaterialTexture(material, aiTextureType_TRANSMISSION, 0, &transmissionTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + transmissionTex.C_Str();
                throw std::runtime_error("unhandled aiTextureType_TRANSMISSION: " + texture);
			}
        }


        textures.resize(texture_helpers.size());
        struct TextureHolder {
            std::unique_ptr<Texture> texture;
            daxa::CommandList cmd_list;
            u32 index;
        };
        std::vector<TextureHolder> texture_holders;
        texture_holders.resize(texture_helpers.size());
        auto load_texture = [&](TextureHelper helper, u32 index){
            auto tex = Texture::load(device, helper.path, helper.format);
            texture_holders[index] = {
                .texture = std::move(tex.first),
                .cmd_list = tex.second,
                .index = index
            };
        };

        ThreadPool pool(std::thread::hardware_concurrency());
        for (u32 i = 0; i < texture_helpers.size(); i++) {
            pool.push_task(load_texture, texture_helpers[i], i);
        }

        pool.wait_for_tasks();

        for(auto& tex : texture_holders) {
            device.submit_commands({
                .command_lists = {std::move(tex.cmd_list)},
            });

            textures[tex.index] = std::move(tex.texture);
        }

        device.wait_idle();
        
        for(usize i = 0; i < scene->mNumMaterials; i++) {
            const aiMaterial* material = scene->mMaterials[i];
            MaterialInfo my_material;
            my_material.has_albedo_texture = 0;
            my_material.has_metallic_texture = 0;
            my_material.has_roughness_texture = 0;
            my_material.has_metallic_roughness_combined = 0;
            my_material.has_normal_map_texture = 0;

            if (aiString diffuseTex{}; aiGetMaterialTexture(material, aiTextureType_DIFFUSE, 0, &diffuseTex) == AI_SUCCESS) {
				std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + diffuseTex.C_Str();
                u32 index = search_textures(texture, daxa::Format::R8G8B8A8_SRGB);
                
                my_material.albedo_texture = {
                    .texture_id = textures[index]->image_id.default_view(),
                    .sampler_id = textures[index]->sampler_id
                };
                my_material.has_albedo_texture = 1;
			}

            if (aiString specularTex{}; aiGetMaterialTexture(material, aiTextureType_SPECULAR, 0, &specularTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + specularTex.C_Str();
                //std::cout << "aiTextureType_SPECULAR: " << texture << std::endl;
                throw std::runtime_error("unhandled aiTextureType_SPECULAR: " + texture);
			}

            if (aiString ambientTex{}; aiGetMaterialTexture(material, aiTextureType_AMBIENT, 0, &ambientTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + ambientTex.C_Str();
                //std::cout << "aiTextureType_AMBIENT: " << texture << std::endl;
                throw std::runtime_error("unhandled aiTextureType_AMBIENT: " + texture);
			}

            if (aiString emissiveTex{}; aiGetMaterialTexture(material, aiTextureType_EMISSIVE, 0, &emissiveTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + emissiveTex.C_Str();
                //std::cout << "aiTextureType_EMISSIVE: " << texture << std::endl;
                throw std::runtime_error("unhandled aiTextureType_EMISSIVE: " + texture);
			}

            if (aiString heightTex{}; aiGetMaterialTexture(material, aiTextureType_HEIGHT, 0, &heightTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + heightTex.C_Str();
                //std::cout << "aiTextureType_HEIGHT: " << texture << std::endl;
                throw std::runtime_error("unhandled aiTextureType_HEIGHT: " + texture);
			}

            if (aiString normalTex{}; aiGetMaterialTexture(material, aiTextureType_NORMALS, 0, &normalTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + normalTex.C_Str();
                //std::cout << "aiTextureType_NORMALS: " << texture << std::endl;
                u32 index = search_textures(texture, daxa::Format::R8G8B8A8_UNORM);
                
                my_material.normal_map_texture = {
                    .texture_id = textures[index]->image_id.default_view(),
                    .sampler_id = textures[index]->sampler_id
                };
                my_material.has_normal_map_texture = 1;
			}

            if (aiString shininessTex{}; aiGetMaterialTexture(material, aiTextureType_SHININESS, 0, &shininessTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + shininessTex.C_Str();
                //std::cout << "aiTextureType_SHININESS: " << texture << std::endl;
                throw std::runtime_error("unhandled aiTextureType_SHININESS: " + texture);
			}

            if (aiString opacityTex{}; aiGetMaterialTexture(material, aiTextureType_OPACITY, 0, &opacityTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + opacityTex.C_Str();
                //std::cout << "aiTextureType_OPACITY: " << texture << std::endl;
                throw std::runtime_error("unhandled aiTextureType_OPACITY: " + texture);
			}

            if (aiString displacementTex{}; aiGetMaterialTexture(material, aiTextureType_DISPLACEMENT, 0, &displacementTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + displacementTex.C_Str();
                //std::cout << "aiTextureType_DISPLACEMENT: " << texture << std::endl;
                throw std::runtime_error("unhandled aiTextureType_DISPLACEMENT: " + texture);
			}

            if (aiString lightmapTex{}; aiGetMaterialTexture(material, aiTextureType_LIGHTMAP, 0, &lightmapTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + lightmapTex.C_Str();
                //std::cout << "aiTextureType_LIGHTMAP: " << texture << std::endl;
                //throw std::runtime_error("unhandled aiTextureType_LIGHTMAP: " + texture);
			}

            if (aiString reflectionTex{}; aiGetMaterialTexture(material, aiTextureType_REFLECTION, 0, &reflectionTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + reflectionTex.C_Str();
                //std::cout << "aiTextureType_REFLECTION: " << texture << std::endl;
                throw std::runtime_error("unhandled aiTextureType_REFLECTION: " + texture);
			}

            if (aiString baseColorTex{}; aiGetMaterialTexture(material, aiTextureType_BASE_COLOR, 0, &baseColorTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + baseColorTex.C_Str();
                //std::cout << "aiTextureType_BASE_COLOR: " << texture << std::endl;
			}

            if (aiString normalCameraTex{}; aiGetMaterialTexture(material, aiTextureType_NORMAL_CAMERA, 0, &normalCameraTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + normalCameraTex.C_Str();
                //std::cout << "aiTextureType_NORMAL_CAMERA: " << texture << std::endl;
                throw std::runtime_error("unhandled aiTextureType_NORMAL_CAMERA: " + texture);
			}

            if (aiString emissionColorTex{}; aiGetMaterialTexture(material, aiTextureType_EMISSION_COLOR, 0, &emissionColorTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + emissionColorTex.C_Str();
                //std::cout << "aiTextureType_EMISSION_COLOR: " << texture << std::endl;
                throw std::runtime_error("unhandled aiTextureType_EMISSION_COLOR: " + texture);
			}

            if (aiString metalnessTex{}; aiGetMaterialTexture(material, aiTextureType_METALNESS, 0, &metalnessTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + metalnessTex.C_Str();
                //std::cout << "aiTextureType_METALNESS: " << texture << std::endl;
                u32 index = search_textures(texture, daxa::Format::R8G8B8A8_UNORM);
                
                my_material.metallic_texture = {
                    .texture_id = textures[index]->image_id.default_view(),
                    .sampler_id = textures[index]->sampler_id
                };
                my_material.has_metallic_texture = 1;
			}

            if (aiString roughnessTex{}; aiGetMaterialTexture(material, aiTextureType_DIFFUSE_ROUGHNESS, 0, &roughnessTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + roughnessTex.C_Str();
                //std::cout << "aiTextureType_DIFFUSE_ROUGHNESS: " << texture << std::endl;
                u32 index = search_textures(texture, daxa::Format::R8G8B8A8_UNORM);
                
                my_material.roughness_texture = {
                    .texture_id = textures[index]->image_id.default_view(),
                    .sampler_id = textures[index]->sampler_id
                };
                my_material.has_roughness_texture = 1;

                if (aiString metalnessTex{}; aiGetMaterialTexture(material, aiTextureType_METALNESS, 0, &metalnessTex) == AI_SUCCESS) {
                std::string _texture = std::filesystem::path{file_path}.parent_path().string() + "/" + metalnessTex.C_Str(); 
                    if(texture == _texture) { my_material.has_metallic_roughness_combined = 1; }
                    //std::cout << "has combined " << my_material.has_metallic_roughness_combined << std::endl;
                }
			}

            if (aiString ambientOcclusionTex{}; aiGetMaterialTexture(material, aiTextureType_AMBIENT_OCCLUSION, 0, &ambientOcclusionTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + ambientOcclusionTex.C_Str();
                //std::cout << "aiTextureType_AMBIENT_OCCLUSION: " << texture << std::endl;
                throw std::runtime_error("unhandled aiTextureType_AMBIENT_OCCLUSION: " + texture);
			}

            if (aiString sheenTex{}; aiGetMaterialTexture(material, aiTextureType_SHEEN, 0, &sheenTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + sheenTex.C_Str();
                //std::cout << "aiTextureType_SHEEN: " << texture << std::endl;
                throw std::runtime_error("unhandled aiTextureType_SHEEN: " + texture);
			}

            if (aiString clearcoatTex{}; aiGetMaterialTexture(material, aiTextureType_CLEARCOAT, 0, &clearcoatTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + clearcoatTex.C_Str();
                //std::cout << "aiTextureType_CLEARCOAT: " << texture << std::endl;
                throw std::runtime_error("unhandled aiTextureType_CLEARCOAT: " + texture);
			}

            if (aiString transmissionTex{}; aiGetMaterialTexture(material, aiTextureType_TRANSMISSION, 0, &transmissionTex) == AI_SUCCESS) {
                std::string texture = std::filesystem::path{file_path}.parent_path().string() + "/" + transmissionTex.C_Str();
                //std::cout << "aiTextureType_TRANSMISSION: " << texture << std::endl;
                throw std::runtime_error("unhandled aiTextureType_TRANSMISSION: " + texture);
			}

            //std::cout << "--------------------" << std::endl;


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
            .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
            .size = static_cast<u32>(sizeof(Vertex) * vertices.size()),
            .debug_name = "",
        });

        index_buffer = device.create_buffer(daxa::BufferInfo{
            .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
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

            auto* buffer_ptr = device.get_host_address_as<Vertex>(vertex_staging_buffer);
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

        material_info_buffer = device.create_buffer(daxa::BufferInfo{
            .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
            .size = static_cast<u32>(sizeof(MaterialInfo) * materials.size()),
            .debug_name = "",
        });

        daxa::BufferId staging_material_info_buffer = device.create_buffer(daxa::BufferInfo{
            .memory_flags = daxa::MemoryFlagBits::HOST_ACCESS_RANDOM,
            .size = static_cast<u32>(sizeof(MaterialInfo) * materials.size()),
            .debug_name = "",
        });

        auto cmd_list = device.create_command_list({
            .debug_name = "",
        });

        cmd_list.destroy_buffer_deferred(staging_material_info_buffer);

        auto* buffer_ptr = device.get_host_address_as<u32>(staging_material_info_buffer);
        std::memcpy(buffer_ptr, materials.data(), sizeof(MaterialInfo) * materials.size());

        cmd_list.pipeline_barrier({
            .awaited_pipeline_access = daxa::AccessConsts::HOST_WRITE,
            .waiting_pipeline_access = daxa::AccessConsts::TRANSFER_READ,
        });

        cmd_list.copy_buffer_to_buffer({
            .src_buffer = staging_material_info_buffer,
            .dst_buffer = material_info_buffer,
            .size = static_cast<u32>(sizeof(MaterialInfo) * materials.size()),
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

    Model::~Model() {
        device.destroy_buffer(face_buffer);
        device.destroy_buffer(index_buffer);
        device.destroy_buffer(material_info_buffer);
    }

    void Model::draw(daxa::CommandList& cmd_list, DepthPrepassPush& draw_push) {
        draw_push.vertex_buffer = device.get_device_address(face_buffer);
        for (auto & primitive : primitives) {
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
        draw_push.material_buffer = device.get_device_address(material_info_buffer);
        draw_push.vertex_buffer = device.get_device_address(face_buffer);
        for (auto & primitive : primitives) {
            draw_push.material_index = primitive.material_index;
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

    void Model::draw(daxa::CommandList& cmd_list, ShadowPush& draw_push) {
        draw_push.vertex_buffer = device.get_device_address(face_buffer);
        for (auto & primitive : primitives) {
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