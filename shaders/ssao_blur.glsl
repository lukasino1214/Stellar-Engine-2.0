#define DAXA_ENABLE_IMAGE_OVERLOADS_BASIC 1
#define DAXA_ENABLE_SHADER_NO_NAMESPACE 1
#include <shared.inl>

DAXA_USE_PUSH_CONSTANT(SSAOBlurPush)

#define sample_texture(tex, uv) texture(tex.texture_id, tex.sampler_id, uv)
#define texture_size(tex, mip) textureSize(tex.texture_id, mip)

#define MATERIAL deref(daxa_push_constant.material_buffer[daxa_push_constant.material_index])
#define LIGHTS_BUFFER deref(daxa_push_constant.light_buffer)
#define CAMERA deref(daxa_push_constant.camera_info)
#define TRANSFORM deref(daxa_push_constant.transform_buffer)
#define VERTEX deref(daxa_push_constant.vertex_buffer[gl_VertexIndex])

#if defined(DRAW_VERT)

layout(location = 0) out f32vec2 out_uv;

void main() {
    out_uv = f32vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = f32vec4(out_uv * 2.0f - 1.0f, 0.0f, 1.0f);
}

#elif defined(DRAW_FRAG)

layout(location = 0) in f32vec2 in_uv;

layout (location = 0) out f32 out_ssao;

void main() {
    const i32 blur_range = 2;
	i32 n = 0;
	f32vec2 texel_size = 1.0 / f32vec2(texture_size(daxa_push_constant.ssao, 0));
	f32 result = 0.0;
	for (i32 x = -blur_range; x < blur_range; x++) {
		for (i32 y = -blur_range; y < blur_range; y++) {
			f32vec2 offset = f32vec2(f32(x), f32(y)) * texel_size;
			result += sample_texture(daxa_push_constant.ssao, in_uv + offset).r;
			n++;
		}
	}
	out_ssao = pow(result / (f32(n)), 3);
}

#endif