#version 450 core

layout (location = 0) in vec3 a_Vertex;

struct Light {
    vec4 position; // (x, y, z, radius) 
    vec4 color;
};
layout (binding = 0) uniform LightBuffer {
    Light light[2048];
};

uniform mat4  u_MVPMatrix;

out vec4 v_Color;

void main(void) {
	vec4 lightPos = light[gl_InstanceID].position;
	vec3 vertex   = a_Vertex * lightPos.w + lightPos.xyz;
	v_Color       = vec4(light[gl_InstanceID].color.rgb, 0.25);
	gl_Position   = u_MVPMatrix * vec4(vertex, 1.0);
}
