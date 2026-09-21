#version 450
layout(push_constant) uniform Transform { mat4 mvp; };
layout(set = 0, binding = 0) uniform Motion {
	mat4 previousMvp;
	vec4 viewport, target, mask, flags;
} u;
layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 texcoord;
layout(location = 3) in vec3 previousPosition;
layout(location = 0) out vec2 uv;
layout(location = 1) out float alpha;
layout(location = 2) out vec4 previousClip;
out gl_PerVertex { vec4 gl_Position; };
void main() {
	gl_Position = mvp * vec4(position, 1);
	previousClip = u.previousMvp * vec4(previousPosition, 1);
	uv = texcoord;
	alpha = color.a;
}
