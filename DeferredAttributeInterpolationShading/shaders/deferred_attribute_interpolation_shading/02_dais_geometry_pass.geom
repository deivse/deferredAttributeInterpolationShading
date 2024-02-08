#version 450 core
layout(triangles, invocations = 1) in;
layout(triangle_strip, max_vertices = 3) out;

in gl_PerVertex { vec4 gl_Position; }
gl_in[];

out gl_PerVertex { vec4 gl_Position; };

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

in int vInstanceID[3];
in uint vNormalSnormOct[3];
in uint vUVsnorm[3];

out flat vec4 vVertices[3];
out flat uint vNormalsSnormOct[3];
out flat uint vUVsSnorm[3];

void main() {
    for (int i = 0; i < gl_in.length(); ++i) {
        gl_Position = gl_in[i].gl_Position;
        vVertices[i] = gl_Position;
        vNormalsSnormOct[i] = vNormalSnormOct[i];
        vUVsSnorm[i] = vUVsnorm[i];
        gl_PrimitiveID
          = gl_PrimitiveIDIn + vInstanceID[0] * int(trianglesPerSphere);
        EmitVertex();
    }
    EndPrimitive();
}
