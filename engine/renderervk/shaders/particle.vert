#version 450
layout(set = 0, binding = 0) uniform Particle {
	vec4 clip[4];
	vec4 uv[4];
	vec4 color;
	vec4 depth;
} u;
layout(location = 0) out vec2 tex_coord;
out gl_PerVertex { vec4 gl_Position; };
void main() {
	const int corner[4] = int[4](0, 1, 3, 2);
	int index = corner[gl_VertexIndex];
	gl_Position = u.clip[index];
	tex_coord = u.uv[index].xy;
}
