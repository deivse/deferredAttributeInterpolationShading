#version 450 core

#ifdef USER_TEST
#endif

layout(location = 0) in vec4 a_Vertex;

layout(binding = 0) uniform SphereCentersBuffer { vec4 sphereOffsets[2048]; };


layout(std140, binding = 2) uniform DSUniforms {
    vec4 cameraPosition;       // size = 16, offset = 0, alignment = 16
    mat4 MVPMatrix;            // size = 64, offset = 16, alignment = 16

    // ---- std140:
    // size = 80, alignment = 16
    // -------------------------
};

out Vertex {
    vec3 position;
    smooth vec3 normal;
    vec2 uv;
};

void main(void) {
    position = a_Vertex.xyz + sphereOffsets[gl_InstanceID].xyz;
    normal = normalize(a_Vertex.xyz);
    uv = a_Vertex.xy;
    gl_Position = MVPMatrix * vec4(position, 1.0);
}
