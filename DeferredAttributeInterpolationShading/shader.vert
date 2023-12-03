#version 450 core

#ifdef USER_TEST
#endif

layout (location = 0) in vec4 a_Vertex;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec4 a_Color;

uniform mat4  u_ProjectionMatrix;
uniform mat4  u_ModelViewMatrix;
uniform int   u_VariableInt;
uniform float u_VariableFloat;

out Vertex {
    vec4 position;
    vec3 normal;
    vec4 color;
};

void main(void) {
    position    = u_ModelViewMatrix * a_Vertex;
    normal      = mat3(u_ModelViewMatrix) * a_Normal;
    color       = a_Color;
    gl_Position = u_ProjectionMatrix * position;
}
