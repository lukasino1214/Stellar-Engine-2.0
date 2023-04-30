#define DAXA_ENABLE_SHADER_NO_NAMESPACE 1
#define DAXA_ENABLE_IMAGE_OVERLOADS_BASIC 1
#include <daxa/daxa.inl>
#include "shared.inl"

DAXA_USE_PUSH_CONSTANT(LinesPush)

#if defined(DRAW_VERT)

void main() {
    gl_Position = CAMERA.projection_matrix * CAMERA.view_matrix * vec4(deref(daxa_push_constant.vertex_buffer[gl_VertexIndex]).position, 1.0);
}

#elif defined(DRAW_FRAG)

layout(location = 0) out f32vec4 out_color;

void main() {
    out_color = vec4(1.0, 0.0, 0.0, 1.0);
}

#endif