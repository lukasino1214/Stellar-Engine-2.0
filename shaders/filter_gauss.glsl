#define DAXA_ENABLE_SHADER_NO_NAMESPACE 1
#define DAXA_ENABLE_IMAGE_OVERLOADS_BASIC 1
#include <daxa/daxa.inl>
#include "shared.inl"


DAXA_USE_PUSH_CONSTANT(GaussPush)

#if defined(DRAW_VERT)
layout(location = 0) out f32vec2 out_uv;

void main() {
    out_uv = f32vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = f32vec4(out_uv * 2.0f - 1.0f, 0.0f, 1.0f);
}
#elif defined(DRAW_FRAG)
layout(location = 0) in f32vec2 in_uv;
layout(location = 0) out f32vec2 out_color;
void main() {
    f32vec2 color = f32vec2(0.0);

	color += sample_texture(daxa_push_constant.src_texture, in_uv + (f32vec2(-3.0) * daxa_push_constant.blur_scale.xy)).xy * ( 1.0 / 64.0 );
	color += sample_texture(daxa_push_constant.src_texture, in_uv + (f32vec2(-2.0) * daxa_push_constant.blur_scale.xy)).xy * ( 6.0 / 64.0 );
	color += sample_texture(daxa_push_constant.src_texture, in_uv + (f32vec2(-1.0) * daxa_push_constant.blur_scale.xy)).xy * (15.0 / 64.0 );
	color += sample_texture(daxa_push_constant.src_texture, in_uv + (f32vec2(+0.0) * daxa_push_constant.blur_scale.xy)).xy * (20.0 / 64.0 );
	color += sample_texture(daxa_push_constant.src_texture, in_uv + (f32vec2(+1.0) * daxa_push_constant.blur_scale.xy)).xy * (15.0 / 64.0 );
	color += sample_texture(daxa_push_constant.src_texture, in_uv + (f32vec2(+2.0) * daxa_push_constant.blur_scale.xy)).xy * ( 6.0 / 64.0 );
	color += sample_texture(daxa_push_constant.src_texture, in_uv + (f32vec2(+3.0) * daxa_push_constant.blur_scale.xy)).xy * ( 1.0 / 64.0 );

	out_color = color;
}
#endif