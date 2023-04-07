#pragma once
#include <daxa/daxa.inl>

struct Vertex {
    daxa_f32vec3 position;
    daxa_f32vec2 uv;
    daxa_f32vec3 normals;
    daxa_f32vec3 tangents;
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
    daxa_f32mat4x4 mvp;
    daxa_BufferPtr(Vertex) vertex_buffer;
    daxa_Image2Df32 texture;
    daxa_SamplerId texture_sampler;
};

struct DepthPrepassPush {
    daxa_f32mat4x4 mvp;
    daxa_BufferPtr(Vertex) vertex_buffer;
};