#define DAXA_ENABLE_SHADER_NO_NAMESPACE 1
#define DAXA_ENABLE_IMAGE_OVERLOADS_BASIC 1
#include <daxa/daxa.inl>
#include "shared.inl"

DAXA_USE_PUSH_CONSTANT(ShadowPush)

#if defined(DRAW_VERT)
void main() {
    gl_Position = daxa_push_constant.light_matrix * TRANSFORM.model_matrix * f32vec4(VERTEX.position, 1.0);
}
#elif defined(DRAW_FRAG)
layout(location = 0) out f32vec2 out_color;
void main() {
    f32 depth = gl_FragCoord.z;

    f32 dx = dFdx(depth);
    f32 dy = dFdy(depth);
    f32 moment2 = depth * depth + 0.25 * (dx * dx + dy * dy);

    out_color = f32vec2(depth, moment2);
}
#endif