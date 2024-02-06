#version 450 core

layout(early_fragment_tests) in;

struct Triangle
{
    // TODO
    vec3 vertices[3];
};

const uint TRIANGLE_SIZE = 8 * 4 * 3;

// TODO: layout, specify binding point in header
layout(std430, binding = 0) buffer TriangleShaderStorageBuffer {
    uint writeIndex;
    Triangle data[10000];
}
triangleSSB;

layout(binding = 0) uniform sampler2D TriangleAddressSampler;
layout(location = 0) out vec4 FragColor;

void main() {
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    uint address = uint(texelFetch(TriangleAddressSampler, pixel, 0).x);

    FragColor = vec4(0, 0, float(address), 1.0);
}
