#version 450

layout(push_constant) uniform Transform { mat4 mvp; };
layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 texcoord;
layout(location = 0) out vec2 uv;
layout(location = 1) out float alpha;
out gl_PerVertex { vec4 gl_Position; };

void main() {
	gl_Position = mvp * vec4(position, 1.0);
	uv = texcoord;
	alpha = color.a;
}
