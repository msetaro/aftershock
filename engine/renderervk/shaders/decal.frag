#version 450
layout(set = 0, binding = 0) uniform Decal {
	vec4 origin, right, down, forward, projection, viewport;
	vec4 volume[3];
	vec4 color, settings, lightDirection, ambient, directed;
} u;
#ifdef MULTISAMPLE
layout(set = 1, binding = 0) uniform sampler2DMS depth_map;
#else
layout(set = 1, binding = 0) uniform sampler2D depth_map;
#endif
layout(set = 2, binding = 0) uniform sampler2D color_map;
layout(set = 3, binding = 0) uniform sampler2D normal_map;
layout(location = 0) in vec2 frag_tex_coord;
layout(location = 0) out vec4 out_color;
vec3 unitOr(vec3 value, vec3 fallback) {
	float lengthSquared = dot(value, value);
	return lengthSquared > 1e-12 ? value * inversesqrt(lengthSquared) : fallback;
}
void main() {
	ivec2 pixel = ivec2(gl_FragCoord.xy);
#ifdef MULTISAMPLE
	float depth = 0.0;
	for (int index = 0; index < textureSamples(depth_map); ++index)
		depth = max(depth, texelFetch(depth_map, pixel, index).r);
#else
	float depth = texelFetch(depth_map, pixel, 0).r;
#endif
	float distanceToEye = u.projection.w / max(depth + u.projection.z, 0.000001);
	vec2 ndc = (gl_FragCoord.xy - u.viewport.xy) / u.viewport.zw * 2.0 - 1.0;
	vec3 world = u.origin.xyz + distanceToEye * (u.forward.xyz + u.right.xyz * ndc.x / u.projection.x + u.down.xyz * ndc.y / u.projection.y);
	vec3 normal = unitOr(cross(dFdx(world), dFdy(world)), normalize(u.volume[2].xyz));
	if (dot(normal, u.origin.xyz - world) < 0.0) normal = -normal;
	vec3 local = vec3(dot(u.volume[0], vec4(world, 1.0)), dot(u.volume[1], vec4(world, 1.0)), dot(u.volume[2], vec4(world, 1.0)));
	if (depth <= 0.0 || any(greaterThan(abs(local), vec3(1.0))) || dot(normal, normalize(u.volume[2].xyz)) < 0.2) discard;
	vec2 uv = local.xy * 0.5 + 0.5;
	vec4 base = texture(color_map, uv) * u.color;
	vec2 xy = (texture(normal_map, uv).xy * 2.0 - 1.0) * u.settings.x;
	vec3 mapped = unitOr(vec3(xy, sqrt(max(0.0, 1.0 - dot(xy, xy)))), vec3(0, 0, 1));
	vec3 tangent = unitOr(u.volume[0].xyz - normal * dot(u.volume[0].xyz, normal), normalize(u.volume[0].xyz));
	vec3 bitangent = cross(normal, tangent);
	if (dot(bitangent, u.volume[1].xyz) < 0.0) bitangent = -bitangent;
	normal = unitOr(tangent * mapped.x + bitangent * mapped.y + normal * mapped.z, normal);
	vec3 light = u.ambient.rgb + u.directed.rgb * max(0.0, dot(normal, unitOr(u.lightDirection.xyz, normal)));
	vec3 linearColor = max(base.rgb * light, vec3(0.0));
	vec3 displayColor = mix(1.055 * pow(linearColor, vec3(1.0 / 2.4)) - 0.055, linearColor * 12.92, lessThanEqual(linearColor, vec3(0.0031308)));
	out_color = vec4(displayColor, base.a);
}
