#version 450
#extension GL_GOOGLE_include_directive : require
#include "direct.glsl"
layout(set = 1, binding = 0) uniform sampler2D baseTexture;
layout(set = 2, binding = 0) uniform sampler2D normalRoughnessTexture;
layout(set = 3, binding = 0) uniform sampler2D emissiveMetallicTexture;
layout(set = 4, binding = 0) uniform sampler2D shadowAtlas;
layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 worldPosition;
layout(location = 2) in vec3 worldNormal;
layout(location = 3) in vec4 worldTangent;
layout(location = 0) out vec4 outColor;
vec3 unitOr(vec3 value, vec3 fallback) {
    float scale = max(max(abs(value.x), abs(value.y)), abs(value.z));
    return scale < 1e-12 ? fallback : normalize(value / scale);
}
float visibility(vec3 delta) {
    if (shadowSettings.x < 0.5) return 1.0;
    int face = 0;
    float grid = 4.0;
    if (lightShape.x < 0.5) {
        vec3 a = abs(delta);
        face = a.x >= a.y && a.x >= a.z ? (delta.x >= 0.0 ? 0 : 1) :
               a.y >= a.z ? (delta.y >= 0.0 ? 2 : 3) : (delta.z >= 0.0 ? 4 : 5);
    } else if (lightShape.x > 1.5) {
        float distance = dot(worldPosition - eye.xyz, viewForward.xyz);
        if (distance > splits.w) return 1.0;
        face = distance <= splits.x ? 0 : distance <= splits.y ? 1 : distance <= splits.z ? 2 : 3;
        grid = 2.0;
    }
    vec4 projected = shadowMatrix[face] * vec4(worldPosition, 1.0);
    if (projected.w <= 0.0) return 1.0;
    vec3 p = projected.xyz / projected.w;
    if (any(greaterThan(abs(p.xy), vec2(1.0))) || p.z < 0.0 || p.z > 1.0) return 1.0;
    float tile = lightShape.z + float(face);
    vec2 corner = vec2(mod(tile, grid), floor(tile / grid)) / grid;
    vec2 pixel = 1.0 / vec2(textureSize(shadowAtlas, 0));
    vec2 sampleUv = corner + (p.xy * 0.5 + 0.5) / grid;
    vec2 lo = corner + pixel * 0.5, hi = corner + 1.0 / grid - pixel * 0.5;
    float result = 0.0;
    for (int y = -1; y <= 1; ++y)
        for (int x = -1; x <= 1; ++x) {
            float depth = texture(shadowAtlas, clamp(sampleUv + vec2(x, y) * pixel, lo, hi)).r;
            result += p.z + shadowSettings.y >= depth ? 1.0 : 0.0;
        }
    return result / 9.0;
}
void main() {
    vec3 n = unitOr(worldNormal, vec3(0, 0, 1));
    vec4 base = texture(baseTexture, uv) * color;
    float roughness = 1.0, metallic = 0.0;
    if (metal.y > 0.5) {
        vec4 nr = texture(normalRoughnessTexture, uv);
        vec3 t = worldTangent.xyz;
        float handedness = worldTangent.w;
        vec3 dpdx = dFdx(worldPosition), dpdy = dFdy(worldPosition);
        vec2 dtdx = dFdx(uv), dtdy = dFdy(uv);
        if (abs(handedness) < 0.5) {
            float det = dtdx.x * dtdy.y - dtdx.y * dtdy.x;
            t = (dpdx * dtdy.y - dpdy * dtdx.y) * sign(det);
            vec3 b = (dpdy * dtdx.x - dpdx * dtdy.x) * sign(det);
            handedness = dot(cross(n, t), b) < 0.0 ? -1.0 : 1.0;
        }
        vec3 fallback = abs(n.z) < 0.99 ? unitOr(cross(vec3(0, 0, 1), n), vec3(1, 0, 0)) : vec3(1, 0, 0);
        t = unitOr(t - n * dot(t, n), fallback);
        vec3 mapped = unitOr(vec3((nr.xy * 2.0 - 1.0) * surface.y, nr.z * 2.0 - 1.0), vec3(0, 0, 1));
        n = unitOr(mat3(t, cross(n, t) * handedness, n) * mapped, n);
        roughness = clamp(nr.a * surface.x, 0.045, 1.0);
        metallic = clamp(texture(emissiveMetallicTexture, uv).a * metal.x, 0.0, 1.0);
    }
    if (!gl_FrontFacing) n = -n;
    vec3 delta = worldPosition - lightPosition.xyz;
    vec3 l = unitOr(-delta, n);
    float attenuation = pow(max(1.0 - dot(delta, delta) / (lightPosition.w * lightPosition.w), 0.0), 2.0);
    if (lightShape.x > 1.5) { l = lightDirection.xyz; attenuation = 1.0; }
    else if (lightShape.x > 0.5) attenuation *= smoothstep(lightDirection.w, lightShape.y, dot(-l, lightDirection.xyz));
    vec3 v = unitOr(eye.xyz - worldPosition, n), h = unitOr(v + l, n);
    float nl = max(dot(n, l), 0.0), nv = max(dot(n, v), 0.0);
    float nh = max(dot(n, h), 0.0), vh = max(dot(v, h), 0.0);
    float a = roughness * roughness, a2 = a * a;
    float d = nh * nh * (a2 - 1.0) + 1.0;
    float distribution = a2 / (3.141592653589793 * d * d);
    float smith = 0.5 / max(nl * sqrt(nv * nv * (1.0 - a2) + a2) + nv * sqrt(nl * nl * (1.0 - a2) + a2), 1e-6);
    vec3 f0 = mix(vec3(0.04), base.rgb, metallic);
    vec3 fresnel = f0 + (1.0 - f0) * pow(1.0 - vh, 5.0);
    vec3 result = (base.rgb * (1.0 - metallic) / 3.141592653589793 + fresnel * distribution * smith) * nl;
    result *= lightColor.rgb * lightColor.a * attenuation * visibility(delta);
    // ponytail: additive display-space contribution on the legacy target; #161
    // replaces this composition with a single HDR exposure/tone-map resolve.
    outColor = vec4(result, 0.0);
}
