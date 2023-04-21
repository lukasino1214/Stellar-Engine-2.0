#define DAXA_ENABLE_SHADER_NO_NAMESPACE 1
#define DAXA_ENABLE_IMAGE_OVERLOADS_BASIC 1
#include <daxa/daxa.inl>
#include "shared.inl"

#define sample_texture(tex, uv) texture(tex.texture_id, tex.sampler_id, uv)
#define texture_size(tex, mip) textureSize(tex.texture_id, mip)

#define MATERIAL deref(daxa_push_constant.material_buffer[daxa_push_constant.material_index])
#define LIGHTS_BUFFER deref(daxa_push_constant.light_buffer)
#define CAMERA deref(daxa_push_constant.camera_info)
#define TRANSFORM deref(daxa_push_constant.transform_buffer)
#define VERTEX deref(daxa_push_constant.vertex_buffer[gl_VertexIndex])

DAXA_USE_PUSH_CONSTANT(CompositionPush)

#if defined(DRAW_VERT)

layout(location = 0) out f32vec2 out_uv;

void main() {
    out_uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(out_uv * 2.0f - 1.0f, 0.0f, 1.0f);
}

#elif defined(DRAW_FRAG)

layout(location = 0) in f32vec2 in_uv;
layout(location = 0) out f32vec4 out_color;

#define sample_shadow(tex, uv, c) textureShadow(tex.texture_id, tex.sampler_id, uv, c)

f32 linstep(f32 low, f32 high, f32 v) {
    return clamp((v-low)/(high-low), 0.0, 1.0);
}

#define SHADOW 0.05

f32 normal_shadow(TextureId shadow_image, f32vec4 shadow_coord, f32vec2 off, f32 bias) {
    f32vec3 proj_coord = shadow_coord.xyz * 0.5 + 0.5;
	return max(sample_shadow(shadow_image, proj_coord.xy + off, shadow_coord.z - bias).r, SHADOW);
}

f32 shadow_pcf(TextureId shadow_image, f32vec4 shadow_coord, f32 bias) {
    i32vec2 tex_dim = texture_size(shadow_image, 0);
	f32 scale = 0.25;
	f32 dx = scale * 1.0 / f32(tex_dim.x);
	f32 dy = scale * 1.0 / f32(tex_dim.y);

	f32 shadow_factor = 0.0;
	i32 count = 0;
	i32 range = 1;
	
	for (i32 x = -range; x <= range; x++) {
		for (i32 y = -range; y <= range; y++) {
			shadow_factor += normal_shadow(shadow_image, shadow_coord, f32vec2(dx*x, dy*y), bias);
			count++;
		}
	
	}
	return (shadow_factor / count);
}

f32 variance_shadow(TextureId shadow_image, f32vec4 shadow_coord) {
    f32vec3 proj_coord = shadow_coord.xyz * 0.5 + 0.5;
	
    f32vec2 moments = sample_texture(shadow_image, proj_coord.xy).xy;
    f32 p = step(shadow_coord.z, moments.x);
    f32 variance = max(moments.y - moments.x * moments.x, 0.00002);
	f32 d = shadow_coord.z - moments.x;
	f32 pMax = linstep(0.1, 1.0, variance / (variance + d*d));

    return max(max(p, pMax), SHADOW);
}

f32vec3 calculate_directional_light(DirectionalLight light, f32vec3 frag_color, f32vec3 normal, f32vec4 position, f32vec3 camera_position) {
    f32vec3 frag_position = position.xyz;
    f32vec3 light_dir = normalize(-light.direction);
    f32 bias = max(0.05 * (1.0 - dot(normal, light_dir)), 0.005); 

    f32vec4 shadow_coord = light.light_matrix * position;
    f32 shadow = 1.0;
    shadow = shadow_pcf(light.shadow_image, shadow_coord / shadow_coord.w, bias);
    
    f32vec3 view_dir = normalize(camera_position - frag_position);
    f32vec3 halfway_dir = normalize(light_dir + view_dir);

    f32 diffuse = max(dot(normal, light_dir), 0);
    f32 normal_half = acos(dot(halfway_dir, normal));
    f32 exponent = normal_half / 1.0;
    exponent = -(exponent * exponent);
    return frag_color * light.color * (diffuse + exp(exponent)) * shadow * light.intensity;
}

f32vec3 calculate_point_light(PointLight light, f32vec3 frag_color, f32vec3 normal, f32vec4 position, f32vec3 camera_position) {
    f32vec3 frag_position = position.xyz;
    f32vec3 light_dir = normalize(light.position - frag_position);

    f32 distance = length(light.position.xyz - frag_position);
    f32 attenuation = 1.0f / (distance * distance);

    f32vec3 view_dir = normalize(camera_position - frag_position);
    f32vec3 halfway_dir = normalize(light_dir + view_dir);

    f32 diffuse = max(dot(normal, light_dir), 0);
    f32 normal_half = acos(dot(halfway_dir, normal));
    f32 exponent = normal_half * 1.0;
    exponent = -(exponent * exponent);
    return frag_color * light.color * (diffuse + exp(exponent)) * attenuation * light.intensity;
}

f32vec3 calculate_spot_light(SpotLight light, f32vec3 frag_color, f32vec3 normal, f32vec4 position, f32vec3 camera_position) {
    f32vec3 frag_position = position.xyz;
    f32vec3 light_dir = normalize(light.position - frag_position);
    f32 bias = max(0.05 * (1.0 - dot(normal, light_dir)), 0.005); 

    f32vec4 shadow_coord = light.light_matrix * position;
    f32 shadow = 1.0;
    shadow = variance_shadow(light.shadow_image, shadow_coord / shadow_coord.w);

    f32 theta = dot(light_dir, normalize(-light.direction)); 
    f32 epsilon = (light.cut_off - light.outer_cut_off);
    f32 intensity = clamp((theta - light.outer_cut_off) / epsilon, 0, 1.0);

    f32 distance = length(light.position - frag_position);
    f32 attenuation = 1.0 / (distance * distance); 

    f32vec3 view_dir = normalize(camera_position - frag_position);
    f32vec3 halfway_dir = normalize(light_dir + view_dir);

    f32 diffuse = max(dot(normal, light_dir), 0);
    f32 normal_half = acos(dot(halfway_dir, normal));
    f32 exponent = normal_half / 1.0;
    exponent = -(exponent * exponent);
    return frag_color * light.color * (diffuse + exp(exponent)) * shadow * attenuation * light.intensity * intensity;
}

f32vec3 get_world_position_from_depth(f32vec2 uv, f32 depth) {
    f32vec4 clipSpacePosition = f32vec4(uv * 2.0 - 1.0, depth, 1.0);
    f32vec4 viewSpacePosition = CAMERA.inverse_projection_matrix * clipSpacePosition;

    viewSpacePosition /= viewSpacePosition.w;
    f32vec4 worldSpacePosition = CAMERA.inverse_view_matrix * viewSpacePosition;

    return worldSpacePosition.xyz;
}

f32vec3 get_view_position_from_depth(f32vec2 uv, f32 depth) {
    f32vec4 clipSpacePosition = f32vec4(uv * 2.0 - 1.0, depth, 1.0);
    f32vec4 viewSpacePosition = CAMERA.inverse_projection_matrix * clipSpacePosition;

    viewSpacePosition /= viewSpacePosition.w;

    return viewSpacePosition.xyz;
}

f32vec3 ACESFilm(f32vec3 x) {
    f32 a = 2.51f;
    f32 b = 0.03f;
    f32 c = 2.43f;
    f32 d = 0.59f;
    f32 e = 0.14f;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

void main() {
    f32vec4 color = sample_texture(daxa_push_constant.albedo, in_uv);
    f32vec4 normal = sample_texture(daxa_push_constant.normal, in_uv);

    f32vec3 ambient = f32vec3(max(daxa_push_constant.ambient, 0.01));
    
    ambient *= color.rgb;
    ambient *= sample_texture(daxa_push_constant.ssao, in_uv).r;

    f32vec4 position = f32vec4(get_world_position_from_depth(in_uv, sample_texture(daxa_push_constant.depth, in_uv).r), 1.0);
    
    f32vec3 camera_position = CAMERA.inverse_view_matrix[3].xyz;

    for(uint i = 0; i < LIGHTS_BUFFER.num_directional_lights; i++) {
        ambient += calculate_directional_light(LIGHTS_BUFFER.directional_lights[i], ambient.rgb, normal.xyz, position, camera_position);
    }

    for(uint i = 0; i < LIGHTS_BUFFER.num_point_lights; i++) {
        ambient += calculate_point_light(LIGHTS_BUFFER.point_lights[i], ambient.rgb, normal.xyz, position, camera_position);
    }

    for(uint i = 0; i < LIGHTS_BUFFER.num_spot_lights; i++) {
        ambient += calculate_spot_light(LIGHTS_BUFFER.spot_lights[i], ambient.rgb, normal.xyz, position, camera_position);
    }

    out_color = f32vec4(ambient, 1.0);
}

#endif