#version 450

layout(set = 0, binding = 0, std140) uniform Mask { vec4 mask; };
layout(set = 1, binding = 0) uniform sampler2D baseMap;
layout(location = 0) in vec2 uv;
layout(location = 1) in float alpha;

void main() {
	// x: material alpha, y: cutoff, z: 0 opaque / 1 GE / 2 LT / 3 GT zero.
	if (mask.z != 0.0) {
		float value = texture(baseMap, uv).a * alpha * mask.x;
		if ((mask.z == 1.0 && value < mask.y) ||
			(mask.z == 2.0 && value >= mask.y) ||
			(mask.z == 3.0 && value <= 0.0))
			discard;
	}
}
