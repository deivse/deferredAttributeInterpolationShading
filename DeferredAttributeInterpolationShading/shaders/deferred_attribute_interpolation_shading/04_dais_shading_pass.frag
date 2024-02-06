#version 450 core

layout(early_fragment_tests) in;

struct Triangle
{
    // TODO
    vec3 vertices[3];
};

layout(binding = 2) uniform SettingsBuffer {
    int bitwiseModHashSize;
    uint trianglesPerSphere;
}
settings;

const uint TRIANGLE_SIZE = 8 * 4 * 3;

// TODO: layout, specify binding point in header
layout(std430, binding = 0) buffer TriangleShaderStorageBuffer {
    Triangle data[10000];
}
triangleSSB;

layout(binding = 0) uniform isampler2D TriangleAddressSampler;
layout(location = 0) out vec4 FragColor;

void main() {
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    int address = texelFetch(TriangleAddressSampler, pixel, 0).r;

    // FragColor = vec4(texelFetch(TriangleAddressSampler, pixel, 0).rgb, 1.0);
    if (address == -1) return;
    FragColor = vec4(triangleSSB.data[address].vertices[1], 1.0);
}
