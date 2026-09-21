#version 450
#extension GL_GOOGLE_include_directive : require
#include "temporal.glsl"
layout(set = 1, binding = 0) uniform sampler2D resolved_map;
layout(set = 2, binding = 0) uniform sampler2D motion_map;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 out_color;
void main() {
	vec4 center = texelFetch(resolved_map, ivec2(gl_FragCoord.xy), 0);
	vec4 motion = texelFetch(motion_map, ivec2(gl_FragCoord.xy), 0);
	vec3 color = center.rgb;
	float weight = 1;
	// Blur only the displayed result; history retains the sharp resolved color.
	if (u.settings.z > 0 && motion.z >= 0 && temporalInside(gl_FragCoord.xy)) {
		vec2 dimensions = vec2(textureSize(resolved_map, 0));
		vec2 delta = motion.xy * dimensions * u.settings.z;
		delta *= min(1, 32 / max(length(delta), .00001));
		for (int i = 1; i < 9; ++i) {
			vec2 point = gl_FragCoord.xy - delta * (float(i) / 8);
			if (!temporalInside(point)) continue;
			vec4 value = texture(resolved_map, point / dimensions);
			if (abs(value.a - center.a) > max(.0005, .02 * center.a)) continue;
			color += value.rgb;
			weight += 1;
		}
	}
	out_color = vec4(temporalDisplay(color / weight) / u.settings.x, 1);
}
