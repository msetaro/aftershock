#version 450

// glTF 2.0 metallic/roughness factors and GGX/Smith/Schlick direct lighting.
// The existing baked light grid supplies ambient and one dominant direction.
layout(set = 0, binding = 0) uniform Parameters {
	vec4 eye, lightDirection, ambient, directed;
	vec4 color, emissiveMetallic, surface;
};
layout(set = 1, binding = 0) uniform sampler2D baseTexture;
layout(set = 2, binding = 0) uniform sampler2D normalRoughnessTexture;
layout(set = 3, binding = 0) uniform sampler2D emissiveMetallicTexture;
layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 objectPosition;
layout(location = 2) in vec3 objectNormal;
layout(location = 3) in vec4 objectTangent;
layout(location = 0) out vec4 outColor;

vec3 unitOr(vec3 value, vec3 fallback) {
	float scale = max(max(abs(value.x), abs(value.y)), abs(value.z));
	if (scale < 1e-12) return fallback;
	return normalize(value / scale);
}

vec3 displayColor(vec3 linearColor) {
	// The legacy UNORM target stores display-space RGB. Encode only PBR output;
	// the shared legacy postprocess and its accepted shader bytes stay unchanged.
	vec3 c = max(linearColor, vec3(0.0));
	return mix(1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055, c * 12.92, lessThanEqual(c, vec3(0.0031308)));
}

void main() {
	vec4 base = texture(baseTexture, uv) * color;
	vec4 nr = texture(normalRoughnessTexture, uv);
	vec4 em = texture(emissiveMetallicTexture, uv);
	vec3 n = unitOr(objectNormal, vec3(0, 0, 1));
	vec3 t = objectTangent.xyz;
	float handedness = objectTangent.w;
	vec3 dpdx = dFdx(objectPosition), dpdy = dFdy(objectPosition);
	vec2 dtdx = dFdx(uv), dtdy = dFdy(uv);
	if (abs(handedness) < 0.5) {
		float determinant = dtdx.x * dtdy.y - dtdx.y * dtdy.x;
		t = (dpdx * dtdy.y - dpdy * dtdx.y) * sign(determinant);
		vec3 b = (dpdy * dtdx.x - dpdx * dtdy.x) * sign(determinant);
		handedness = dot(cross(n, t), b) < 0.0 ? -1.0 : 1.0;
	}
	vec3 fallback = abs(n.z) < 0.99 ? unitOr(cross(vec3(0, 0, 1), n), vec3(1, 0, 0)) : vec3(1, 0, 0);
	t = unitOr(t - n * dot(t, n), fallback);
	vec3 mapped = unitOr(vec3((nr.xy * 2.0 - 1.0) * surface.y, nr.z * 2.0 - 1.0), vec3(0, 0, 1));
	n = unitOr(mat3(t, cross(n, t) * handedness, n) * mapped, n);
	if (!gl_FrontFacing) n = -n;
	int flags = int(surface.w);
	if ((flags & 8) != 0 && base.a < surface.z) discard;
	float opacity = (flags & 4) != 0 ? base.a : 1.0;
	if ((flags & 2) != 0) {
		outColor = vec4(displayColor(base.rgb), opacity);
		return;
	}
	float metallic = clamp(em.a * emissiveMetallic.a, 0.0, 1.0);
	float roughness = clamp(nr.a * surface.x, 0.045, 1.0);
	vec3 v = unitOr(eye.xyz - objectPosition, n);
	vec3 l = unitOr(lightDirection.xyz, n);
	vec3 h = unitOr(v + l, n);
	float nl = max(dot(n, l), 0.0), nv = max(dot(n, v), 0.0);
	float nh = max(dot(n, h), 0.0), vh = max(dot(v, h), 0.0);
	const float pi = 3.141592653589793;
	float a = roughness * roughness, a2 = a * a;
	float d = nh * nh * (a2 - 1.0) + 1.0;
	float distribution = a2 / (pi * d * d);
	float visibility = 0.5 / max(nl * sqrt(nv * nv * (1.0 - a2) + a2) + nv * sqrt(nl * nl * (1.0 - a2) + a2), 1e-6);
	vec3 f0 = mix(vec3(0.04), base.rgb, metallic);
	vec3 fresnel = f0 + (1.0 - f0) * pow(1.0 - vh, 5.0);
	vec3 diffuse = base.rgb * (1.0 - metallic);
	vec3 direct = ((1.0 - fresnel) * diffuse / pi + fresnel * distribution * visibility) * nl * directed.rgb;
	vec3 result = diffuse * ambient.rgb + direct + em.rgb * emissiveMetallic.rgb;
	outColor = vec4(displayColor(result), opacity);
}
