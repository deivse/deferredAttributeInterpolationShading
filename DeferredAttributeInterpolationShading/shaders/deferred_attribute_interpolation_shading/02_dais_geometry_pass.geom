#version 450 core
layout(triangles, invocations = 1) in;
layout(triangle_strip, max_vertices = 3) out;

in gl_PerVertex { vec4 gl_Position; }
gl_in[];

out gl_PerVertex { vec4 gl_Position; };

layout(binding = 2) uniform SettingsBuffer {
    int bitwiseModHashSize;
    uint trianglesPerSphere;
}
settings;

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
        gl_PrimitiveID = gl_PrimitiveIDIn
                         + vInstanceID[0] * int(settings.trianglesPerSphere);
        EmitVertex();
    }
    EndPrimitive();
}
