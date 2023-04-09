#pragma once
#include <daxa/daxa.inl>

struct TextureId {
    daxa_Image2Df32 texture_id;
    daxa_SamplerId sampler_id;
};

struct MaterialInfo {
    TextureId albedo_texture;
    daxa_f32vec4 albedo_factor;
    daxa_u32 has_albedo_texture;
    TextureId metallic_texture;
    daxa_f32 metallic_factor;
    daxa_u32 has_metallic_texture;
    TextureId roughness_texture;
    daxa_f32 roughness_factor;
    daxa_u32 has_roughness_texture;
    daxa_u32 has_metallic_roughness_combined;
    TextureId normal_map_texture;
    daxa_u32 has_normal_map_texture;
};

DAXA_ENABLE_BUFFER_PTR(MaterialInfo)

struct TransformInfo {
    daxa_f32mat4x4 model_matrix;
    daxa_f32mat4x4 normal_matrix;
};

DAXA_ENABLE_BUFFER_PTR(TransformInfo)

struct CameraInfo {
    daxa_f32mat4x4 projection_matrix;
    daxa_f32mat4x4 inverse_projection_matrix;
    daxa_f32mat4x4 view_matrix;
    daxa_f32mat4x4 inverse_view_matrix;
    daxa_f32vec3 position;
};

DAXA_ENABLE_BUFFER_PTR(CameraInfo)

struct DirectionalLight {
    daxa_f32vec3 direction;
    daxa_f32vec3 color;
    daxa_f32 intensity;
};

struct PointLight {
    daxa_f32vec3 position;
    daxa_f32vec3 color;
    daxa_f32 intensity;
};

struct SpotLight {
    daxa_f32vec3 position;
    daxa_f32vec3 direction;
    daxa_f32vec3 color;
    daxa_f32 intensity;
    daxa_f32 cut_off;
    daxa_f32 outer_cut_off;
};

#define MAX_LIGHTS 128
struct LightBuffer {
    DirectionalLight directional_lights[MAX_LIGHTS];
    daxa_i32 num_directional_lights;
    PointLight point_lights[MAX_LIGHTS];
    daxa_i32 num_point_lights;
    SpotLight spot_lights[MAX_LIGHTS];
    daxa_i32 num_spot_lights;
};

DAXA_ENABLE_BUFFER_PTR(LightBuffer)

struct Vertex {
    daxa_f32vec3 position;
    daxa_f32vec2 uv;
    daxa_f32vec3 normal;
    daxa_f32vec3 tangent;
};

DAXA_ENABLE_BUFFER_PTR(Vertex)

struct _Vertex {
    daxa_f32vec3 position;
    daxa_f32vec2 uv;
};

DAXA_ENABLE_BUFFER_PTR(_Vertex)

struct TexturePush {
    daxa_Image2Df32 texture;
    daxa_SamplerId texture_sampler;
};

struct DrawPush {
    daxa_BufferPtr(Vertex) vertex_buffer;
    daxa_BufferPtr(MaterialInfo) material_buffer;
    daxa_i32 material_index;
    daxa_BufferPtr(TransformInfo) transform_buffer;
    daxa_BufferPtr(LightBuffer) light_buffer;
    daxa_BufferPtr(CameraInfo) camera_info;
};

struct DepthPrepassPush {
    daxa_BufferPtr(CameraInfo) camera_info;
    daxa_BufferPtr(Vertex) vertex_buffer;
    daxa_BufferPtr(TransformInfo) transform_buffer;
};



#define sample_texture(tex, uv) texture(tex.texture_id, tex.sampler_id, uv)

#define MATERIAL deref(daxa_push_constant.material_buffer[daxa_push_constant.material_index])
#define LIGHT_BUFFER deref(daxa_push_constant.light_buffer)
#define CAMERA deref(daxa_push_constant.camera_info)
#define TRANSFORM deref(daxa_push_constant.transform_buffer)
#define VERTEX deref(daxa_push_constant.vertex_buffer[gl_VertexIndex])