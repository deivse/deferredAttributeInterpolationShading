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
flat out uint v_LightID;

const float   Scale = 3.0;	// TODO: To simulate conservative rasterization

void main(void) {
	v_LightID     = gl_InstanceID;
	vec4 lightPos = light[gl_InstanceID].position;
	vec3 vertex   = a_Vertex * (lightPos.w * Scale) + lightPos.xyz;
	gl_Position   = u_MVPMatrix * vec4(vertex, 1.0);
}
