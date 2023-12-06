#version 450 core

layout (location = 0) in vec4 a_Vertex;

layout (location = 0) uniform int  u_SpherePerRow = 1;
layout (location = 3) uniform vec3 u_SphereCenter;

uniform mat4 u_MVPMatrix;

out vec2 v_TexCoord;
out vec3 v_Normal;
out vec4 v_Vertex;

void main(void) {
    v_Vertex    = vec4(a_Vertex.xyz + u_SphereCenter, 1.0);
    v_Normal    = normalize(a_Vertex.xyz);
    v_TexCoord  = a_Vertex.xy;
    gl_Position = u_MVPMatrix * v_Vertex;
}
