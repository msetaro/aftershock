#version 450
layout(location = 0) in vec2 frag_tex_coord;
layout(location = 0) out vec4 out_color;
layout(set = 0, binding = 0) uniform Occlusion {
	vec4 projection; // x/y scale, reversed depth coefficients
	vec4 viewport; // normalized origin/extent
	vec4 settings; // world radius, strength, normal bias, unused
	vec4 texel; // full depth reciprocal size, AO reciprocal size
} u;
#ifdef MULTISAMPLE
layout(set = 1, binding = 0) uniform sampler2DMS depth_map;
#else
layout(set = 1, binding = 0) uniform sampler2D depth_map;
#endif
#ifdef FILTER
layout(set = 2, binding = 0) uniform sampler2D occlusion_map;
#endif
float depthAt(vec2 uv) {
	ivec2 size = textureSize(depth_map
#ifndef MULTISAMPLE
		, 0
#endif
	);
	ivec2 pixel = clamp(ivec2(uv * vec2(size)), ivec2(0), size - 1);
#ifdef MULTISAMPLE
	float depth = 0.0;
	for (int sampleIndex = 0; sampleIndex < textureSamples(depth_map); ++sampleIndex)
		depth = max(depth, texelFetch(depth_map, pixel, sampleIndex).r);
	return depth;
#else
	return texelFetch(depth_map, pixel, 0).r;
#endif
}
vec3 positionAt(vec2 uv) {
	float z = -u.projection.w / (depthAt(uv) + u.projection.z);
	vec2 ndc = (uv - u.viewport.xy) / u.viewport.zw * 2.0 - 1.0;
	return vec3(ndc * -z / u.projection.xy, z);
}
void main() {
	vec2 uv = frag_tex_coord;
	float depth = depthAt(uv);
	if (depth <= 0.0 || depth >= 0.5 || any(lessThan(uv, u.viewport.xy)) || any(greaterThan(uv, u.viewport.xy + u.viewport.zw))) {
		out_color = vec4(1.0);
		return;
	}
	vec3 position = positionAt(uv);
#ifdef FILTER
	float sum = 0.0, weight = 0.0;
	for (int y = -1; y <= 1; ++y) {
		for (int x = -1; x <= 1; ++x) {
			vec2 at = uv + vec2(x, y) * u.texel.zw;
			float w = exp(-abs(positionAt(at).z - position.z) / max(u.settings.x * 0.1, 0.01));
			w *= (x == 0 ? 2.0 : 1.0) * (y == 0 ? 2.0 : 1.0);
			sum += texture(occlusion_map, at).r * w;
			weight += w;
		}
	}
	float ao = sum / max(weight, 0.001);
#else
	vec3 right = positionAt(uv + vec2(u.texel.x, 0.0)) - position;
	vec3 left = position - positionAt(uv - vec2(u.texel.x, 0.0));
	vec3 down = positionAt(uv + vec2(0.0, u.texel.y)) - position;
	vec3 up = position - positionAt(uv - vec2(0.0, u.texel.y));
	vec3 normal = cross(abs(right.z) < abs(left.z) ? right : left, abs(down.z) < abs(up.z) ? down : up);
	normal /= max(length(normal), 0.000001);
	if (dot(normal, -position) < 0.0) normal = -normal;
	vec2 radius = abs(u.projection.xy) * u.viewport.zw * u.settings.x / max(-2.0 * position.z, 0.001);
	float sum = 0.0;
	// A fixed kernel avoids temporal noise and needs no random texture/history.
	for (int i = 0; i < 16; ++i) {
		float angle = float(i) * 2.39996323;
		float distanceScale = sqrt((float(i) + 0.5) / 16.0);
		vec2 at = uv + vec2(cos(angle), sin(angle)) * radius * distanceScale;
		at = clamp(at, u.viewport.xy + u.texel.xy, u.viewport.xy + u.viewport.zw - u.texel.xy);
		vec3 delta = positionAt(at) - position;
		float distanceToSample = length(delta);
		sum += max(dot(normal, delta) - u.settings.z, 0.0) / max(distanceToSample, 0.001)
			* max(1.0 - distanceToSample / u.settings.x, 0.0);
	}
	float ao = clamp(1.0 - sum * u.settings.y * (4.0 / 16.0), 0.0, 1.0);
#endif
	out_color = vec4(ao, ao, ao, 1.0);
}
