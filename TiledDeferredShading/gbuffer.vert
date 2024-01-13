#version 450 core

layout (location = 0) in vec4 a_Vertex;
layout (location = 1) in vec3 a_SphereCenter;

uniform mat4 u_MVPMatrix;

out vec2 v_TexCoord;
out vec3 v_Normal;
out vec4 v_Vertex;

void main(void) {
    v_Vertex    = vec4(a_Vertex.xyz + a_SphereCenter, 1.0);
    v_Normal    = normalize(a_Vertex.xyz);
    v_TexCoord  = a_Vertex.xy;
    gl_Position = u_MVPMatrix * v_Vertex;
}
