#define DAXA_ENABLE_IMAGE_OVERLOADS_BASIC 1
#define DAXA_ENABLE_SHADER_NO_NAMESPACE 1
#include <shared.inl>

DAXA_USE_PUSH_CONSTANT(DrawPush)

#define sample_texture(tex, uv) texture(tex.texture_id, tex.sampler_id, uv)
#define get_material deref(daxa_push_constant.material_buffer[daxa_push_constant.material_index])

#define get_light_buffer deref(daxa_push_constant.light_buffer)

#if defined(DRAW_VERT)

layout(location = 0) out f32vec3 out_normal;
layout(location = 1) out f32vec2 out_uv;

void main() {
    out_normal = f32mat3x3(deref(daxa_push_constant.transform_buffer).normal_matrix) * deref(daxa_push_constant.vertex_buffer[gl_VertexIndex]).normal;
    out_uv = deref(daxa_push_constant.vertex_buffer[gl_VertexIndex]).uv;
    gl_Position = daxa_push_constant.mvp * deref(daxa_push_constant.transform_buffer).model_matrix * vec4(deref(daxa_push_constant.vertex_buffer[gl_VertexIndex]).position, 1.0);
}

#elif defined(DRAW_FRAG)

layout(location = 0) out f32vec4 color;

layout(location = 0) in f32vec3 in_normal;
layout(location = 1) in f32vec2 in_uv;

void main() {
    f32vec3 tex_col = sample_texture(get_material.albedo_texture, in_uv).rgb;

    for(i32 i = 0; i < deref(daxa_push_constant.light_buffer).num_directional_lights; i++) {
        tex_col *= get_light_buffer.directional_lights[i].color * get_light_buffer.directional_lights[i].intensity * dot(get_light_buffer.directional_lights[i].direction, in_normal);
    }

    // f32vec3 tex_col = f32vec3(1.0);
    // if(get_material.has_normal_map_texture == 1) {
    //     tex_col = sample_texture(get_material.normal_map_texture, in_uv).rgb;
    // }

    // if(get_material.has_metallic_roughness_combined == 1) {
    //     tex_col = sample_texture(get_material.metallic_texture, in_uv).rgb;
    // }

    color = f32vec4(tex_col, 1.0);
}

#endif