#define DAXA_ENABLE_SHADER_NO_NAMESPACE 1
#define DAXA_ENABLE_IMAGE_OVERLOADS_BASIC 1
#include <daxa/daxa.inl>
#include "shared.inl"

#define sample_texture(tex, uv) texture(tex.texture_id, tex.sampler_id, uv)

#define MATERIAL deref(daxa_push_constant.material_buffer[daxa_push_constant.material_index])
#define LIGHTS_BUFFER deref(daxa_push_constant.light_buffer)
#define CAMERA deref(daxa_push_constant.camera_info)
#define TRANSFORM deref(daxa_push_constant.transform_buffer)
#define VERTEX deref(daxa_push_constant.vertex_buffer[gl_VertexIndex])


DAXA_USE_PUSH_CONSTANT(DrawPush)

#if defined(DRAW_VERT)

layout(location = 0) out f32vec2 out_uv;
layout(location = 1) out f32vec3 out_position;
layout(location = 2) out f32vec3 out_normal;

void main() {
    out_normal = f32mat3x3(TRANSFORM.normal_matrix) * VERTEX.normal;
    out_uv = VERTEX.uv;
    out_position = (TRANSFORM.model_matrix * f32vec4(VERTEX.position, 1.0)).xyz;
    gl_Position = CAMERA.projection_matrix * CAMERA.view_matrix * TRANSFORM.model_matrix * f32vec4(VERTEX.position, 1.0);
}

#elif defined(DRAW_FRAG)

layout(location = 0) in f32vec2 in_uv;
layout(location = 1) in f32vec3 in_position;
layout(location = 2) in f32vec3 in_normal;

layout(location = 0) out f32vec4 out_albedo;
layout(location = 1) out f32vec4 out_normal;

f32vec3 apply_normal_mapping(TextureId normal_map, f32vec3 position, f32vec3 normal, f32vec2 uv) {
    f32vec3 tangent_normal = sample_texture(normal_map, uv).xyz * 2.0 - 1.0;

    f32vec3 Q1  = dFdx(position);
    f32vec3 Q2  = dFdy(position);
    f32vec2 st1 = dFdx(uv);
    f32vec2 st2 = dFdy(uv);

    f32vec3 N   = normalize(normal);

    f32vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    f32vec3 B  = normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangent_normal);
}

void main() {
    f32vec3 color = f32vec3(1.0);
    if(MATERIAL.has_albedo_texture == 1) {
        color = sample_texture(MATERIAL.albedo_texture, in_uv).rgb;
    }

    out_albedo = f32vec4(color, 1.0);

    f32vec3 normal = normalize(in_normal);
    if(MATERIAL.has_normal_map_texture == 1) {
        normal = apply_normal_mapping(MATERIAL.normal_map_texture, in_position, normal, in_uv);
    }

    out_normal = f32vec4(normal, 1.0);
}

#endif