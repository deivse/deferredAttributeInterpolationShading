#version 450 core

layout(location = 0) in vec4 a_Vertex;
layout(binding = 0) uniform SphereCentersBuffer { vec4 sphereOffsets[2048]; };

uniform mat4 u_MVPMatrix;

void main(void) {
    gl_Position = u_MVPMatrix
                  * vec4(a_Vertex.xyz + sphereOffsets[gl_InstanceID].xyz, 1.0);
}
