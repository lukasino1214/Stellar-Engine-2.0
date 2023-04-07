#define DAXA_ENABLE_IMAGE_OVERLOADS_BASIC 1
#define DAXA_ENABLE_SHADER_NO_NAMESPACE 1
#include <shared.inl>

DAXA_USE_PUSH_CONSTANT(DepthPrepassPush)

#if defined(DRAW_VERT)

void main() {
    gl_Position = daxa_push_constant.mvp * vec4(deref(daxa_push_constant.vertex_buffer[gl_VertexIndex]).position, 1.0);
}

#elif defined(DRAW_FRAG)

void main() {}

#endif