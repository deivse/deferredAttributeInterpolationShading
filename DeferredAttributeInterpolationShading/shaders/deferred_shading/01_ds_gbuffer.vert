#version 450 core

#ifdef USER_TEST
#endif

layout(location = 0) in vec4 a_Vertex;

layout(binding = 0) uniform SphereCentersBuffer { vec4 sphereOffsets[2048]; };

uniform mat4 u_MVPMatrix;

out Vertex {
    vec3 position;
    smooth vec3 normal;
    vec2 uv;
};

void main(void) {
    position = a_Vertex.xyz + sphereOffsets[gl_InstanceID].xyz;
    normal = normalize(a_Vertex.xyz);
    uv = a_Vertex.xy;
    gl_Position = u_MVPMatrix * vec4(position, 1.0);
}
