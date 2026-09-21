layout(set = 0, binding = 0) uniform Temporal {
	vec4 origin, right, down, forward;
	vec4 projection, jitter, viewport;
	mat4 previous;
	vec4 settings; // overbright scale, valid history, motion blur strength
} u;
vec3 temporalLinear(vec3 c) {
	c = max(c, vec3(0));
	return mix(pow((c + .055) / 1.055, vec3(2.4)), c / 12.92, lessThanEqual(c, vec3(.04045)));
}
vec3 temporalDisplay(vec3 c) {
	c = max(c, vec3(0));
	return mix(1.055 * pow(c, vec3(1.0 / 2.4)) - .055, c * 12.92, lessThanEqual(c, vec3(.0031308)));
}
bool temporalInside(vec2 pixel) {
	return all(greaterThanEqual(pixel, u.viewport.xy + .5)) && all(lessThanEqual(pixel, u.viewport.xy + u.viewport.zw - .5));
}
