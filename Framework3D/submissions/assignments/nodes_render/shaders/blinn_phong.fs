#version 430 core

// Define a uniform struct for lights
struct Light {
    // The matrices are used for shadow mapping. You need to fill it according to how we are filling it when building the normal maps (node_render_shadow_mapping.cpp).
    // Now, they are filled with identity matrix. You need to modify C++ code innode_render_deferred_lighting.cpp.
    // Position and color are filled.
    mat4 light_projection;
    mat4 light_view;
    vec3 position;
    float radius;
    vec3 color; // Just use the same diffuse and specular color.
    int shadow_map_id;
};

layout(binding = 0) buffer lightsBuffer {
Light lights[4];
};

uniform vec2 iResolution;

uniform sampler2D diffuseColorSampler;
uniform sampler2D normalMapSampler; // You should apply normal mapping in rasterize_impl.fs
uniform sampler2D metallicRoughnessSampler;
uniform sampler2DArray shadow_maps;
uniform sampler2D position;

// uniform float alpha;
uniform vec3 camPos;

uniform int light_count;

layout(location = 0) out vec4 Color;

void main() {
vec2 uv = gl_FragCoord.xy / iResolution;

vec3 pos = texture2D(position,uv).xyz;
vec3 normal = texture2D(normalMapSampler,uv).xyz;

vec4 metalnessRoughness = texture2D(metallicRoughnessSampler,uv);
float metal = metalnessRoughness.x;
float roughness = metalnessRoughness.y;

vec3 diffuse_color = texture2D(diffuseColorSampler, uv).xyz;

const vec3 ambient_light = vec3(0.1, 0.1, 0.1);

Color = vec4(0,0,0,1);
for(int i = 0; i < light_count; i ++) {

vec3 this_color = vec3(0,0,0);

float alpha = 1.0f / (roughness * roughness + 0.0001);
vec3 pos_light = lights[i].position;
vec3 color_light = lights[i].color;
vec3 dir2light = normalize(pos_light - pos);
float dist_light_surf = length(pos_light - pos);
vec3 dir2eye = normalize(camPos - pos);
vec3 hvec = normalize(dir2light + dir2eye);
color_light /= 4 * 3.1415926 * dist_light_surf * dist_light_surf;

vec3 diffuse_term = color_light * max(0, dot(normal, dir2light));
diffuse_term = diffuse_term * diffuse_color * (1.0-roughness);
vec3 spec_term = pow(max(dot(normal, hvec), 0.0), alpha)* diffuse_color *roughness *color_light;
this_color = spec_term + diffuse_term;

vec4 homopos = vec4(pos, 1.0);
vec4 homo_lightspace_pos = lights[i].light_projection * lights[i].light_view * homopos;
homo_lightspace_pos /=homo_lightspace_pos.w;
float depth_light_space = homo_lightspace_pos.z - 0.01;
float shadow_scale = 0;
float shadow_map_value = texture(shadow_maps, vec3(homo_lightspace_pos.xy*0.5 + 0.5, lights[i].shadow_map_id)).x;
if(shadow_map_value > depth_light_space) shadow_scale = 1;

this_color *= shadow_scale;
Color += vec4(this_color,1);

}
Color += vec4(diffuse_color*ambient_light, 1);
}