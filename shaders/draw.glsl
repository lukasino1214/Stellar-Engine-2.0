#define DAXA_ENABLE_IMAGE_OVERLOADS_BASIC 1
#define DAXA_ENABLE_SHADER_NO_NAMESPACE 1
#include <shared.inl>

DAXA_USE_PUSH_CONSTANT(DrawPush)

#if defined(DRAW_VERT)

layout(location = 0) out f32vec2 out_uv;

void main() {
    out_uv = deref(daxa_push_constant.vertex_buffer[gl_VertexIndex]).uv;
    gl_Position = daxa_push_constant.mvp * vec4(deref(daxa_push_constant.vertex_buffer[gl_VertexIndex]).position, 1.0);
}

#elif defined(DRAW_FRAG)

layout(location = 0) out f32vec4 color;

layout(location = 0) in f32vec2 in_uv;

void main() {
    f32vec3 tex_col = texture(daxa_push_constant.texture, daxa_push_constant.texture_sampler, in_uv).rgb;
    color = f32vec4(tex_col, 1.0);
}

#endif