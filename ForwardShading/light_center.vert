#version 450 core

struct Light {
    vec4 position; // (x, y, z, radius) 
    vec4 color;
};
layout (binding = 0) uniform LightBuffer {
    Light light[2048];
};

uniform mat4  u_MVPMatrix;
uniform int   u_VariableInt;
uniform float u_VariableFloat;

out vec4 v_Color;

void main(void) {
	vec3 vertex = light[gl_VertexID].position.xyz;
	v_Color     = light[gl_VertexID].color;
	gl_Position = u_MVPMatrix * vec4(vertex, 1.0);
}
