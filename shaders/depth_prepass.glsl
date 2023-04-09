#define DAXA_ENABLE_IMAGE_OVERLOADS_BASIC 1
#define DAXA_ENABLE_SHADER_NO_NAMESPACE 1
#include <shared.inl>

DAXA_USE_PUSH_CONSTANT(DepthPrepassPush)

#if defined(DRAW_VERT)

void main() {
    gl_Position = CAMERA.projection_matrix * CAMERA.view_matrix * TRANSFORM.model_matrix * f32vec4(VERTEX.position, 1.0);
}

#elif defined(DRAW_FRAG)

void main() {}

#endif