#version 450 core

#ifdef USER_TEST
#endif

layout (location = 0) out vec4 FragColor;

in vec2 v_TexCoord;
in vec3 v_Normal;
in vec4 v_Vertex;

layout (binding = 0)  uniform sampler2D u_Texture;
layout (location = 1) uniform vec3      u_CameraPosition;
layout (location = 2) uniform uint      u_NumLights;

uniform int   u_VariableInt;
uniform float u_VariableFloat;

struct Light {
    vec4 position; // (x, y, z, radius) 
    vec4 color;    // (diffuse.rgb, specular)
};
layout (binding = 0) uniform LightBuffer {
    Light light[2048];
};

// Simple phong lighting
vec3 lighting(in uint lightIdx, in vec3 vertex, in vec3 normal, in vec4 diffSpecColor) {
    vec4 lightPos = light[lightIdx].position;

    vec3 lightDir   = lightPos.xyz - vertex;
    float distance  = length(lightDir);
    // Check if point is out of the light's range
    if (distance > lightPos.w) 
        return vec3(0.0);

    // Get light color
    vec4 color = light[lightIdx].color;

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
    vec3 normal        = normalize(v_Normal);
    vec4 diffSpecColor = texture(u_Texture, v_TexCoord); // (diffuse.rgb, specular)

    // sum lighting
    vec3 color = vec3(0.0);
    for (uint i = 0; i < u_NumLights; i++) {
        color += lighting(i, v_Vertex.xyz, normal, diffSpecColor);
    }

    FragColor = vec4(color, 1.0);
}
