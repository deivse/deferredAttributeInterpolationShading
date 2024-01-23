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
layout(location = 0) uniform vec3 u_CameraPosition;
layout(location = 1) uniform uint u_NumLights;

layout(binding = 0) uniform sampler2D ColorSampler;
layout(binding = 1) uniform sampler2D NormalSampler;
layout(binding = 2) uniform sampler2D VertexSampler;

// Simple phong lighting
vec3 calculateLightContribution(in uint lightIdx, in vec3 vertex,
                                in vec3 normal, in vec4 diffSpecColor) {
    vec4 lightPos = lights[lightIdx].position;

    // Check if point is out of the light's range
    vec3 lightDir = lightPos.xyz - vertex;
    float distance = length(lightDir);
    if (distance > lightPos.w) return vec3(0.0);

    // Get light color
    vec4 color = lights[lightIdx].color;

    // Calculate light attenuation factor
    // float attenuation = 1.0;
    float attenuation = 1.0 - distance / lightPos.w;

    // Calculate diffuse color component
    lightDir = normalize(lightDir);
    float NdotL = max(dot(normal, lightDir), 0.0);
    vec3 lightContribution = color.rgb * diffSpecColor.rgb * NdotL;

    // Calculate specular color component
    vec3 viewDir = normalize(u_CameraPosition - vertex);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float NdotH = pow(max(dot(normal, halfwayDir), 0.0), 5.0);
    // Multiply by NdotL to 
    lightContribution += color.aaa * diffSpecColor.a * NdotH * NdotL;

    return lightContribution * attenuation;
}

void main(void) {
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    vec3 normal = texelFetch(NormalSampler, pixel, 0).xyz;
    // (diffuse.rgb, specular)
    vec4 diffSpecColor = texelFetch(ColorSampler, pixel, 0);
    vec4 position = texelFetch(VertexSampler, pixel, 0);

    FragColor = vec4(vec3(0.0), 1.0);
    for (uint i = 0; i < u_NumLights; ++i) {
        vec3 lightContribution
          = calculateLightContribution(i, position.xyz, normal, diffSpecColor);
        FragColor.rgb += lightContribution;
    }
}
