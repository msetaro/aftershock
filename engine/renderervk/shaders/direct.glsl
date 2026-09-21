layout(set = 0, binding = 0) uniform Parameters {
    mat4 model, shadowMatrix[6];
    vec4 eye, lightPosition, lightDirection, lightColor;
    vec4 lightShape, shadowSettings, splits, viewForward;
    vec4 color, surface, metal;
};
