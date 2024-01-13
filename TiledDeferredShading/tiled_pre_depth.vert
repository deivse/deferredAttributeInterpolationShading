#version 450 core

layout (location = 0) in vec4 a_Vertex;
layout (location = 1) in vec3 a_SphereCenter;

uniform mat4 u_MVPMatrix;

void main(void) {
    gl_Position = u_MVPMatrix * vec4(a_Vertex.xyz + a_SphereCenter, 1.0);
}
