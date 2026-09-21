#version 450
layout(set = 0, binding = 0) uniform Particle {
	vec4 clip[4];
	vec4 uv[4];
	vec4 color;
	vec4 depth; // reversed projection coefficients and world-space fade distance
} u;
#ifdef MULTISAMPLE
layout(set = 1, binding = 0) uniform sampler2DMS depth_map;
#else
layout(set = 1, binding = 0) uniform sampler2D depth_map;
#endif
layout(set = 2, binding = 0) uniform sampler2D color_map;
layout(location = 0) in vec2 tex_coord;
layout(location = 0) out vec4 out_color;
void main() {
	ivec2 pixel = ivec2(gl_FragCoord.xy);
#ifdef MULTISAMPLE
	float sceneDepth = 0.0;
	for (int index = 0; index < textureSamples(depth_map); ++index)
		sceneDepth = max(sceneDepth, texelFetch(depth_map, pixel, index).r);
#else
	float sceneDepth = texelFetch(depth_map, pixel, 0).r;
#endif
	float sceneDistance = u.depth.y / max(sceneDepth + u.depth.x, 0.000001);
	float particleDistance = u.depth.y / max(gl_FragCoord.z + u.depth.x, 0.000001);
	float fade = clamp((sceneDistance - particleDistance) / u.depth.z, 0.0, 1.0);
	out_color = texture(color_map, tex_coord) * u.color;
	out_color.a *= fade;
}
