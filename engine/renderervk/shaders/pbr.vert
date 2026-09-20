#version 450

layout(push_constant) uniform Transform { mat4 mvp; };
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec4 tangent;
layout(location = 0) out vec2 uv;
layout(location = 1) out vec3 objectPosition;
layout(location = 2) out vec3 objectNormal;
layout(location = 3) out vec4 objectTangent;
out gl_PerVertex { vec4 gl_Position; };

void main() {
	gl_Position = mvp * vec4(position, 1.0);
	uv = texcoord;
	objectPosition = position;
	objectNormal = normal;
	objectTangent = tangent;
}
