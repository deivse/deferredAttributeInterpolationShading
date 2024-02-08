#version 450 core

layout(location = 0) in vec4 a_Vertex;
layout(binding = 0) uniform SphereCentersBuffer { vec4 sphereOffsets[2048]; };

layout(std140, binding = 1) uniform DAISUniforms {
    vec4 cameraPosition;       // size = 16, offset = 0, alignment = 16
    mat4 MVPMatrix;            // size = 64, offset = 16, alignment = 16
    mat4 MVPMatrixInv;         // size = 64, offset = 80, alignment = 16
    vec4 Viewport;             // size = 16, offset = 144, alignment = 16
    int bitwiseModHashSize;    // size = 4, offset = 160, alignment = 4
    uint trianglesPerSphere;   // size = 4, offset = 164, alignment = 4
    float projectionMatrix_32; // size = 4, offset = 168, alignment = 4
    float projectionMatrix_22; // size = 4, offset = 172, alignment = 4

    // ---- std140:
    // size = 176, alignment = 16
    // -------------------------
};

out int vInstanceID;

out uint vNormalSnormOct;
out uint vUVsnorm;

// Returns ±1
vec2 signNotZero(vec2 v) {
    return vec2((v.x >= 0.0) ? +1.0 : -1.0, (v.y >= 0.0) ? +1.0 : -1.0);
}
// Assume normalized input. Output is on [-1, 1] for each component.
vec2 float32x3_to_oct(in vec3 v) {
    // Project the sphere onto the octahedron, and then onto the xy plane
    vec2 p = v.xy * (1.0 / (abs(v.x) + abs(v.y) + abs(v.z)));
    // Reflect the folds of the lower hemisphere over the diagonals
    return (v.z <= 0.0) ? ((1.0 - abs(p.yx)) * signNotZero(p)) : p;
}

void main(void) {
    gl_Position
      = MVPMatrix * vec4(a_Vertex.xyz + sphereOffsets[gl_InstanceID].xyz, 1.0);
    vNormalSnormOct = packSnorm2x16(float32x3_to_oct(a_Vertex.xyz));
    vUVsnorm = packSnorm2x16(a_Vertex.xy);

    // Used in geometry shader to calculate globally unique triangle id
    vInstanceID = gl_InstanceID;
}
