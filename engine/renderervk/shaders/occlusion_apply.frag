#version 450
layout(location = 0) in vec2 frag_tex_coord;
layout(location = 0) out vec4 out_color;
layout(set = 1, binding = 0) uniform sampler2D occlusion_map;
void main() {
	float ao = texture(occlusion_map, frag_tex_coord).r;
	out_color = vec4(ao, ao, ao, 1.0);
}
