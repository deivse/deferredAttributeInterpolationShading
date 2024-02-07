#version 450 core

layout(early_fragment_tests) in;

struct Triangle
{
    // in std430, vec3s are not padded to vec4 when in an array or structure.
    vec3 vertices[3];        // size = 36, offset = 0
    uint normalsSnormOct[3]; // size = 12, offset = 36
    uint UVsUnorm[3];        // size = 12, offset = 48

    // struct size = 72 bytes (60 w/o padding), alignment = 36
    // 12 bytes padding since 60 % 36 == -12, where 36 is the alignment of
    // biggest member.
};

// Returns ±1
vec2 signNotZero(vec2 v) {
    return vec2((v.x >= 0.0) ? +1.0 : -1.0, (v.y >= 0.0) ? +1.0 : -1.0);
}

vec3 oct_to_float32x3(vec2 e) {
    vec3 v = vec3(e.xy, 1.0 - abs(e.x) - abs(e.y));
    if (v.z < 0) v.xy = (1.0 - abs(v.yx)) * signNotZero(v.xy);
    return normalize(v);
}

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
    vec3 normal
      = oct_to_float32x3(unpackSnorm2x16(triangles[index].normalsSnormOct[0]));
    vec2 uv = unpackUnorm2x16(triangles[index].UVsUnorm[0]); 
    FragColor = vec4(uv, 0.0, 1.0);
}
