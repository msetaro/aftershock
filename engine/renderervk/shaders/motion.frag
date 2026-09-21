#version 450
layout(set = 0, binding = 0) uniform Motion {
	mat4 previousMvp;
	vec4 viewport, target, mask, flags;
} u;
layout(set = 1, binding = 0) uniform sampler2D baseMap;
layout(location = 0) in vec2 uv;
layout(location = 1) in float alpha;
layout(location = 2) in vec4 previousClip;
layout(location = 0) out vec4 out_motion;
void main() {
	if (u.mask.z != 0) {
		float value = texture(baseMap, uv).a * alpha * u.mask.x;
		if ((u.mask.z == 1 && value < u.mask.y) || (u.mask.z == 2 && value >= u.mask.y) || (u.mask.z == 3 && value <= 0)) discard;
	}
	out_motion = vec4(0, 0, -1, gl_FragCoord.z);
	if (u.flags.z == 0 || previousClip.w <= 0) return;
	vec3 clip = previousClip.xyz / previousClip.w;
	vec2 pixel = u.viewport.xy + (clip.xy * .5 + .5) * u.viewport.zw;
	if (clip.z < 0 || clip.z > 1 || any(lessThan(pixel, u.viewport.xy + .5)) || any(greaterThan(pixel, u.viewport.xy + u.viewport.zw - .5))) return;
	out_motion = vec4((gl_FragCoord.xy - pixel) / u.target.xy, clip.z * u.flags.x + u.flags.y, gl_FragCoord.z);
}
