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
void main() {}
#endif