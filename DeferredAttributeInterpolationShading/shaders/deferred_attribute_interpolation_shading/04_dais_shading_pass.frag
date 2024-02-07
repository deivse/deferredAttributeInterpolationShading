#version 450 core

layout(early_fragment_tests) in;

struct Triangle
{
    // TODO
    vec3 vertices[3];
};

// TODO: layout, specify binding point in header
layout(std430, binding = 0) buffer TriangleShaderStorageBuffer {
    Triangle triangles[];
};

layout(binding = 0) uniform isampler2D TriangleIndexSampler;
layout(location = 0) out vec4 FragColor;

void main() {
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    int index = texelFetch(TriangleIndexSampler, pixel, 0).r;

    // FragColor = vec4(texelFetch(TriangleIndexSampler, pixel, 0).rgb, 1.0);
    if (index == -1) return;
    FragColor = vec4(triangles[index].vertices[1], 1.0);
}
