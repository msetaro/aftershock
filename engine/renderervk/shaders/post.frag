#version 450
layout(set = 0, binding = 0) uniform Post {
	vec4 curve; // exposure multiplier, sharpen, vignette, grain
	vec4 lens; // LUT strength, focus distance/range, depth blur radius
	vec4 projection; // reversed-depth coefficients, time, legacy overbright scale
	vec4 viewport; // normalized world viewport
} u;
layout(set = 1, binding = 0) uniform sampler2D color_map;
#ifdef MULTISAMPLE
layout(set = 2, binding = 0) uniform sampler2DMS depth_map;
#else
layout(set = 2, binding = 0) uniform sampler2D depth_map;
#endif
layout(set = 3, binding = 0) uniform sampler2D grade_map;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 out_color;
vec3 linearColor(vec3 c) {
	c = max(c, vec3(0));
	return mix(pow((c + .055) / 1.055, vec3(2.4)), c / 12.92, lessThanEqual(c, vec3(.04045)));
}
vec3 displayColor(vec3 c) {
	c = max(c, vec3(0));
	return mix(1.055 * pow(c, vec3(1.0 / 2.4)) - .055, c * 12.92, lessThanEqual(c, vec3(.0031308)));
}
vec3 scene(vec2 point) {
	vec2 halfTexel = .5 / vec2(textureSize(color_map, 0));
	return linearColor(texture(color_map, clamp(point, u.viewport.xy + halfTexel, u.viewport.xy + u.viewport.zw - halfTexel)).rgb * u.projection.w);
}
void main() {
	if (any(lessThan(uv, u.viewport.xy)) || any(greaterThan(uv, u.viewport.xy + u.viewport.zw))) {
		out_color = texture(color_map, uv);
		return;
	}
	vec2 texel = 1.0 / vec2(textureSize(color_map, 0));
	vec3 color = scene(uv);
	if (u.lens.w > 0) {
		ivec2 pixel = ivec2(gl_FragCoord.xy);
#ifdef MULTISAMPLE
		float depth = 0;
		for (int i = 0; i < textureSamples(depth_map); ++i)
			depth = max(depth, texelFetch(depth_map, pixel, i).r);
#else
		float depth = texelFetch(depth_map, pixel, 0).r;
#endif
		float distance = u.projection.y / max(depth + u.projection.x, .000001);
		float radius = clamp(abs(distance - u.lens.y) / u.lens.z, 0, 1) * u.lens.w;
		// Bounded nine-tap presentation blur; no extra render targets.
		for (int y = -1; y <= 1; ++y)
			for (int x = -1; x <= 1; ++x)
				if (x != 0 || y != 0) color += scene(uv + vec2(x,y) * texel * radius);
		color /= 9;
	}
	if (u.curve.y > 0) {
		vec3 neighbors = scene(uv + vec2(texel.x,0)) + scene(uv - vec2(texel.x,0)) + scene(uv + vec2(0,texel.y)) + scene(uv - vec2(0,texel.y));
		color = max(color + u.curve.y * (color - neighbors * .25), vec3(0));
	}
	color *= u.curve.x;
	// Krzysztof Narkowicz's CC0 ACES fit (2016), not full ACES color management.
	// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
	color = clamp((color * (2.51 * color + .03)) / (color * (2.43 * color + .59) + .14), 0, 1);
	color = displayColor(color);
	if (u.lens.x > 0) {
		vec3 cell = clamp(color, 0, 1) * 15;
		float slice = floor(cell.b);
		vec2 p = vec2((cell.r + slice * 16 + .5) / 256, (cell.g + .5) / 16);
		vec3 a = textureLod(grade_map, p, 0).rgb;
		vec3 b = textureLod(grade_map, p + vec2(min(slice + 1, 15) - slice, 0) / 16, 0).rgb;
		color = mix(color, mix(a, b, fract(cell.b)), u.lens.x);
	}
	vec2 local = (uv - u.viewport.xy) / u.viewport.zw * 2 - 1;
	color *= 1 - u.curve.z * smoothstep(.15, 1.5, dot(local, local));
	uint noise = uint(gl_FragCoord.x) * 1973u + uint(gl_FragCoord.y) * 9277u + uint(u.projection.z) * 26699u;
	noise = (noise ^ (noise >> 13)) * 1274126177u;
	color += (float(noise & 65535u) / 65535 - .5) * u.curve.w * .12;
	out_color = vec4(clamp(color, 0, 1) / u.projection.w, 1);
}
