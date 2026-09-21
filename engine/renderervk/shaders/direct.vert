#version 450
#extension GL_GOOGLE_include_directive : require
#include "direct.glsl"
layout(push_constant) uniform Transform { mat4 mvp; };
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec4 tangent;
layout(location = 0) out vec2 uv;
layout(location = 1) out vec3 worldPosition;
layout(location = 2) out vec3 worldNormal;
layout(location = 3) out vec4 worldTangent;
out gl_PerVertex { vec4 gl_Position; };
void main() {
    gl_Position = mvp * vec4(position, 1.0);
    uv = texcoord;
    worldPosition = (model * vec4(position, 1.0)).xyz;
    worldNormal = transpose(inverse(mat3(model))) * normal;
    worldTangent = vec4(mat3(model) * tangent.xyz, tangent.w * sign(determinant(mat3(model))));
}
