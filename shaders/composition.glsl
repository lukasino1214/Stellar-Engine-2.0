#define DAXA_ENABLE_SHADER_NO_NAMESPACE 1
#define DAXA_ENABLE_IMAGE_OVERLOADS_BASIC 1
#include <daxa/daxa.inl>
#include "shared.inl"

#define sample_texture(tex, uv) texture(tex.texture_id, tex.sampler_id, uv)

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

f32vec3 calculate_directional_light(DirectionalLight light, f32vec3 frag_color, f32vec3 normal, f32vec4 position, f32vec3 camera_position) {
    f32vec3 frag_position = position.xyz;
    f32vec3 light_dir = normalize(-light.direction);
    f32 bias = max(0.05 * (1.0 - dot(normal, light_dir)), 0.005); 
    
    f32vec3 view_dir = normalize(camera_position - frag_position);
    f32vec3 halfway_dir = normalize(light_dir + view_dir);

    f32 diffuse = max(dot(normal, light_dir), 0);
    f32 normal_half = acos(dot(halfway_dir, normal));
    f32 exponent = normal_half / 1.0;
    exponent = -(exponent * exponent);
    return frag_color * light.color * (diffuse + exp(exponent)) * light.intensity;
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
    f32 shadow = 1.0;
    f32vec3 frag_position = position.xyz;
    f32vec3 light_dir = normalize(light.position - frag_position);
    f32 bias = max(0.05 * (1.0 - dot(normal, light_dir)), 0.005); 

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
    return frag_color * light.color * (diffuse + exp(exponent)) * attenuation * light.intensity * intensity;
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

void main() {
    f32vec4 color = sample_texture(daxa_push_constant.albedo, in_uv);
    f32vec4 normal = sample_texture(daxa_push_constant.normal, in_uv);

    f32vec3 ambient = f32vec3(0.05);
    
    ambient *= color.rgb;

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