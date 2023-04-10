#define DAXA_ENABLE_SHADER_NO_NAMESPACE 1
#define DAXA_ENABLE_IMAGE_OVERLOADS_BASIC 1
#include <daxa/daxa.inl>
#include "shared.inl"

DAXA_USE_PUSH_CONSTANT(BillboardPush)

#if defined(DRAW_VERT)

const vec2 OFFSETS[6] = vec2[](
    vec2(-1.0, -1.0),
    vec2(-1.0, 1.0),
    vec2(1.0, -1.0),
    vec2(1.0, -1.0),
    vec2(-1.0, 1.0),
    vec2(1.0, 1.0)
);

const vec2 UVS[6] = vec2[](
    vec2(1.0, 1.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(0.0, 1.0),
    vec2(1.0, 0.0),
    vec2(0.0, 0.0)
);

layout(location = 0) out f32vec2 out_uv;

const float LIGHT_RADIUS = 0.25;

void main() {
    out_uv = UVS[gl_VertexIndex];
    vec2 frag_offset = OFFSETS[gl_VertexIndex];
    vec3 camera_right = {CAMERA.view_matrix[0][0], CAMERA.view_matrix[1][0], CAMERA.view_matrix[2][0]};
    vec3 camera_up = {CAMERA.view_matrix[0][1], CAMERA.view_matrix[1][1], CAMERA.view_matrix[2][1]};

    vec3 position = daxa_push_constant.position.xyz
        + LIGHT_RADIUS * frag_offset.x * camera_right
        + LIGHT_RADIUS * frag_offset.y * camera_up;

    gl_Position = CAMERA.projection_matrix * CAMERA.view_matrix * vec4(position, 1.0);
}

#elif defined(DRAW_FRAG)

layout(location = 0) in f32vec2 in_uv;
layout(location = 0) out f32vec4 out_color;

void main() {
    f32vec4 tex_col = sample_texture(daxa_push_constant.texture, in_uv);
    if(tex_col.a < 0.5) { discard; } 
    out_color = vec4(sample_texture(daxa_push_constant.texture, in_uv).rgb, 1.0);
}

#endif