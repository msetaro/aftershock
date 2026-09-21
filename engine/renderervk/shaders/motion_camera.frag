#version 450
#extension GL_GOOGLE_include_directive : require
#include "temporal.glsl"
layout(set = 1, binding = 0) uniform sampler2D depth_map;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 out_motion;
void main() {
	float depth = texelFetch(depth_map, ivec2(gl_FragCoord.xy), 0).r;
	out_motion = vec4(0, 0, -1, depth);
	if (u.settings.y == 0 || depth <= 0 || !temporalInside(gl_FragCoord.xy)) return;
	float distance = u.projection.w / max(depth + u.projection.z, .000001);
	vec2 ndc = (gl_FragCoord.xy - u.viewport.xy) / u.viewport.zw * 2 - 1;
	vec3 world = u.origin.xyz + distance * (u.forward.xyz + u.right.xyz * (ndc.x + u.jitter.x) / u.projection.x + u.down.xyz * (ndc.y + u.jitter.y) / u.projection.y);
	vec4 previous = u.previous * vec4(world, 1);
	if (previous.w <= 0) return;
	vec3 clip = previous.xyz / previous.w;
	vec2 previousPixel = u.viewport.xy + (clip.xy * .5 + .5) * u.viewport.zw;
	if (clip.z < 0 || clip.z > 1 || !temporalInside(previousPixel)) return;
	out_motion = vec4(uv - previousPixel / vec2(textureSize(depth_map, 0)), clip.z, depth);
}
