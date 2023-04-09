#define DAXA_ENABLE_IMAGE_OVERLOADS_BASIC 1
#define DAXA_ENABLE_SHADER_NO_NAMESPACE 1
#include <shared.inl>

DAXA_USE_PUSH_CONSTANT(DrawPush)

#define sample_texture(tex, uv) texture(tex.texture_id, tex.sampler_id, uv)

#define MATERIAL deref(daxa_push_constant.material_buffer[daxa_push_constant.material_index])
#define LIGHTS_BUFFER deref(daxa_push_constant.light_buffer)
#define CAMERA deref(daxa_push_constant.camera_info)
#define TRANSFORM deref(daxa_push_constant.transform_buffer)
#define VERTEX deref(daxa_push_constant.vertex_buffer[gl_VertexIndex])

#if defined(DRAW_VERT)

layout(location = 0) out f32vec3 out_normal;
layout(location = 1) out f32vec2 out_uv;
layout(location = 2) out f32vec3 out_position;

void main() {
    out_normal = f32mat3x3(TRANSFORM.normal_matrix) * VERTEX.normal;
    out_uv = VERTEX.uv;
    out_position = (TRANSFORM.model_matrix * f32vec4(VERTEX.position, 1.0)).xyz;
    gl_Position = CAMERA.projection_matrix * CAMERA.view_matrix * TRANSFORM.model_matrix * f32vec4(VERTEX.position, 1.0);
}

#elif defined(DRAW_FRAG)

layout(location = 0) out f32vec4 color;

layout(location = 0) in f32vec3 in_normal;
layout(location = 1) in f32vec2 in_uv;
layout(location = 2) in f32vec3 in_position;

f32vec3 calculate_directional_light(DirectionalLight light, f32vec3 frag_color, f32vec3 normal, f32vec4 position, f32vec3 camera_position) {
    //f32vec4 shadow_coord = light.light_matrix * position;
    f32 shadow = 1.0;
    f32vec3 frag_position = position.xyz;
    f32vec3 light_dir = normalize(-light.direction);
    f32 bias = max(0.05 * (1.0 - dot(normal, light_dir)), 0.005); 

    /*if(light.shadow_type == 1) {
        shadow = shadow_pcf(light.shadow_image, shadow_coord / shadow_coord.w, bias);
    } else if(light.shadow_type == 2) {
        shadow = variance_shadow(light.shadow_image, shadow_coord / shadow_coord.w);
    }*/
    
    
    f32vec3 view_dir = normalize(camera_position - frag_position);
    f32vec3 halfway_dir = normalize(light_dir + view_dir);

    f32 diffuse = max(dot(normal, light_dir), 0);
    f32 normal_half = acos(dot(halfway_dir, normal));
    f32 exponent = normal_half / 1.0;
    exponent = -(exponent * exponent);
    return frag_color * light.color * ((diffuse + exp(exponent)) * shadow) * light.intensity;
}

f32vec3 calculate_point_light(PointLight light, f32vec3 frag_color, f32vec3 normal, f32vec4 position, f32vec3 camera_position) {
    f32 shadow = 1.0;
    f32vec3 frag_position = position.xyz;
    f32vec3 light_dir = normalize(light.position - frag_position);

    /*if(light.shadow_type == 2) {
        shadow = variance_shadow_point(light.shadow_image, position.xyz - light.position, far_plane);
    }*/
    
    f32 distance = length(light.position.xyz - frag_position);
    f32 attenuation = 1.0f / (distance * distance);

    f32vec3 view_dir = normalize(camera_position - frag_position);
    f32vec3 halfway_dir = normalize(light_dir + view_dir);

    f32 diffuse = max(dot(normal, light_dir), 0);
    f32 normal_half = acos(dot(halfway_dir, normal));
    f32 exponent = normal_half * 1.0;
    exponent = -(exponent * exponent);
    return frag_color * light.color * ((diffuse + exp(exponent)) * shadow) * attenuation * light.intensity;
}

f32vec3 calculate_spot_light(SpotLight light, f32vec3 frag_color, f32vec3 normal, f32vec4 position, f32vec3 camera_position) {
    //f32vec4 shadow_coord = light.light_matrix * position;
    f32 shadow = 1.0;
    f32vec3 frag_position = position.xyz;
    f32vec3 light_dir = normalize(light.position - frag_position);
    f32 bias = max(0.05 * (1.0 - dot(normal, light_dir)), 0.005); 

    /*if(light.shadow_type == 1) {
        shadow = shadow_pcf(light.shadow_image, shadow_coord / shadow_coord.w, bias);
    } else if(light.shadow_type == 2) {
        shadow = variance_shadow(light.shadow_image, shadow_coord / shadow_coord.w);
    }*/
    

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
    return frag_color * light.color * ((diffuse + exp(exponent)) * shadow) * attenuation * light.intensity * intensity;
}

f32vec3 apply_normal_mapping(TextureId normal_map, f32vec3 position, f32vec3 normal, f32vec2 uv) {
    f32vec3 tangent_normal = sample_texture(normal_map, uv).xyz * 2.0 - 1.0;

    f32vec3 Q1  = dFdx(in_position);
    f32vec3 Q2  = dFdy(in_position);
    f32vec2 st1 = dFdx(uv);
    f32vec2 st2 = dFdy(uv);

    f32vec3 N   = normalize(normal);

    f32vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    f32vec3 B  = normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangent_normal);
}

void main() {
    f32vec3 tex_col = sample_texture(MATERIAL.albedo_texture, in_uv).rgb;

    f32vec3 ambient = f32vec3(0.05);
    
    ambient *= tex_col.rgb;

    f32vec4 position = f32vec4(in_position, 1.0);
    
    f32vec3 camera_position = CAMERA.inverse_view_matrix[3].xyz;

    f32vec3 normal = in_normal;
    if(MATERIAL.has_normal_map_texture == 1) {
        normal = apply_normal_mapping(MATERIAL.normal_map_texture, in_position, normal, in_uv);
    }

    for(uint i = 0; i < LIGHTS_BUFFER.num_directional_lights; i++) {
        ambient += calculate_directional_light(LIGHTS_BUFFER.directional_lights[i], ambient.rgb, normal.xyz, position, camera_position);
    }

    for(uint i = 0; i < LIGHTS_BUFFER.num_point_lights; i++) {
        ambient += calculate_point_light(LIGHTS_BUFFER.point_lights[i], ambient.rgb, normal.xyz, position, camera_position);
    }

    for(uint i = 0; i < LIGHTS_BUFFER.num_spot_lights; i++) {
        ambient += calculate_spot_light(LIGHTS_BUFFER.spot_lights[i], ambient.rgb, normal.xyz, position, camera_position);
    }

    color = f32vec4(ambient, 1.0);
    //color = f32vec4(normal * 0.5 + 0.5, 1.0);
}

#endif