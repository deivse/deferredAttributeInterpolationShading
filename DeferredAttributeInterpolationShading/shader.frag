#version 450 core

#ifdef USER_TEST
#endif

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 SecondColorTex;

uniform int   u_VariableInt;
uniform float u_VariableFloat;
uniform mat4  u_ModelViewMatrix;

in Vertex {
    vec4 position;
    vec3 normal;
    vec4 color;
};

const vec4 c_LightWorldPos = vec4(0.0, 0.9, 1.0, 1.0);

void main(void) {
    vec4 lightViewPos = u_ModelViewMatrix * c_LightWorldPos;
    
    // Compute fragment color
    float NdotL = max(0.0, dot(normalize(lightViewPos.xyz - position.xyz), normalize(normal)));
    FragColor  = vec4(color.rgb * NdotL, color.a);
    SecondColorTex = vec4(vec3(NdotL), 1);;
}
