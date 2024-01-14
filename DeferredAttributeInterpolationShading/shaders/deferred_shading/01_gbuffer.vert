#version 450 core

#ifdef USER_TEST
#endif

layout (location = 0) in vec4 a_Vertex;

layout(binding = 1) uniform SphereCentersBuffer { vec4 sphereOffsets[2048]; };

uniform mat4  u_MVPMatrix;

out Vertex {
    vec4 position;
    smooth vec3 normal;
    vec4 color;
};

void main(void) {
    position    = vec4(a_Vertex.xyz + sphereOffsets[gl_InstanceID].xyz, 1.0);
    normal      = normalize(a_Vertex.xyz);
    color       = vec4(vec3(0), 1.0);
    gl_Position = u_MVPMatrix * position;
}
