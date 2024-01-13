#version 450 core

#ifdef USER_TEST
#endif

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 SecondColorTex;

struct Light
{
    vec4 position; // (x, y, z, radius)
    vec4 color;
};
layout(binding = 0) uniform LightBuffer { Light lights[2048]; };

uniform int u_VariableInt;
uniform float u_VariableFloat;
uniform mat4 u_ModelViewMatrix;
uniform vec3 u_CameraPosition = vec3(0.0, 0.0, 0.0);

in Vertex {
    vec4 position;
    vec3 normal;
    vec4 color;
};

const vec4 c_LightWorldPos = vec4(0.0, 0.9, 1.0, 1.0);

// Simple phong lighting
vec3 calculateLightContribution(in uint lightIdx, in vec3 vertex, in vec3 normal, in vec4 diffSpecColor) {
    vec4 lightPos = lights[lightIdx].position;

    // Check if point is out of the light's range
    vec3 lightDir   = lightPos.xyz - vertex;
    float distance  = length(lightDir);
    if (distance > lightPos.w) 
        return vec3(0.0);

    // Get light color
    vec4 color = lights[lightIdx].color;

    // Calculate light attenuation factor
    float attenuation = 1.0 - distance / lightPos.w;

    // Calculate diffuse color component
    lightDir      = normalize(lightDir);
    float NdotL   = max(dot(normal, lightDir), 0.0);
    vec3  diffuse = color.rgb * diffSpecColor.rgb * NdotL;

    // Calculate specular color component
    vec3  viewDir    = normalize(u_CameraPosition - vertex);
    vec3  halfwayDir = normalize(lightDir + viewDir);  
    float NdotH      = pow(max(dot(normal, halfwayDir), 0.0), 16.0);
    vec3  specular   = color.aaa * diffSpecColor.a * NdotH;         

    // Sum
    return (diffuse + specular) * attenuation;
}


void main(void) {
    FragColor = vec4(vec3(0), 1.0);
    for (uint i = 0; i < 2048; ++i) {
        vec3 lightContribution = calculateLightContribution(i, position.xyz, normal, color);
        FragColor.rgb += lightContribution;
    }
}
