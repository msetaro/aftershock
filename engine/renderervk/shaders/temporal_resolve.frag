#version 450
#extension GL_GOOGLE_include_directive : require
#include "temporal.glsl"
layout(set = 1, binding = 0) uniform sampler2D color_map;
layout(set = 2, binding = 0) uniform sampler2D motion_map;
layout(set = 3, binding = 0) uniform sampler2D history_map;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 out_color;
vec3 current(ivec2 pixel) {
	return temporalLinear(texelFetch(color_map, pixel, 0).rgb * u.settings.x);
}
void main() {
	ivec2 pixel = ivec2(gl_FragCoord.xy);
	vec4 motion = texelFetch(motion_map, pixel, 0);
	vec3 color = current(pixel);
	out_color = vec4(color, motion.w);
	// Never fetch uninitialized history on a first frame, cut or rejected object.
	if (u.settings.y == 0 || motion.z < 0 || !temporalInside(gl_FragCoord.xy)) return;
	vec2 previous = uv - motion.xy;
	vec2 dimensions = vec2(textureSize(color_map, 0));
	if (!temporalInside(previous * dimensions)) return;
	float priorDepth = texelFetch(history_map, ivec2(previous * dimensions), 0).a;
	if (abs(priorDepth - motion.z) > max(.0005, .02 * motion.z)) return;
	vec3 low = color, high = color;
	ivec2 lower = ivec2(u.viewport.xy), upper = ivec2(u.viewport.xy + u.viewport.zw) - 1;
	for (int y = -1; y <= 1; ++y)
		for (int x = -1; x <= 1; ++x) {
			vec3 sampleColor = current(clamp(pixel + ivec2(x,y), lower, upper));
			low = min(low, sampleColor);
			high = max(high, sampleColor);
		}
	vec3 history = clamp(texture(history_map, previous).rgb, low, high);
	float weight = mix(.9, .5, clamp(length(motion.xy * dimensions) / 32, 0, 1));
	out_color.rgb = mix(color, history, weight);
}
