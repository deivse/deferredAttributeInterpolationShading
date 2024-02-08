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

void main(void) {
    gl_Position
      = MVPMatrix * vec4(a_Vertex.xyz + sphereOffsets[gl_InstanceID].xyz, 1.0);
}
