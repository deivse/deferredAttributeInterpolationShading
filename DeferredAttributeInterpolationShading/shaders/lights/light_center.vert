#version 450 core

struct Light
{
    vec4 position; // (x, y, z, radius)
    vec4 color;
};
layout(std430, binding = 2) buffer LightBuffer {
    uint numLights; // size = 4, offset = 0
    Light lights[]; // size = 32, offset = 16, alignment = 16
};

uniform mat4 u_MVPMatrix;
uniform int u_VariableInt;
uniform float u_VariableFloat;

out vec4 v_Color;

void main(void) {
    vec3 vertex = lights[gl_VertexID].position.xyz;
    v_Color = lights[gl_VertexID].color;
    gl_Position = u_MVPMatrix * vec4(vertex, 1.0);
}
