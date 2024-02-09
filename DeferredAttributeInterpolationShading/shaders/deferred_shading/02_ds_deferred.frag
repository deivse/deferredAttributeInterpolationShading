#version 450 core

#ifdef USER_TEST
#endif

layout(location = 0) out vec4 FragColor;

struct Light
{
    vec4 position; // (x, y, z, radius)
    vec4 color;
};
layout(std430, binding = 2) buffer LightBuffer {
    uint numLights; // size = 4, offset = 0
    Light lights[]; // size = 32, offset = 16, alignment = 16
};

layout(std140, binding = 2) uniform DSUniforms {
    vec4 cameraPosition; // size = 16, offset = 0, alignment = 16
    mat4 MVPMatrix;      // size = 64, offset = 16, alignment = 16
    uint MSAASamples;    // size = 4, offset = 80, alignment = 4

    // ---- std140:
    // size = 84, alignment = 16
    // -------------------------
};

layout(binding = 0) uniform sampler2D ColorSampler;
layout(binding = 1) uniform sampler2D NormalSampler;
layout(binding = 2) uniform sampler2D VertexSampler;

layout(binding = 4) uniform sampler2DMS ColorSamplerMS;
layout(binding = 5) uniform sampler2DMS NormalSamplerMS;
layout(binding = 6) uniform sampler2DMS VertexSamplerMS;

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
    vec3 viewDir = normalize(cameraPosition.xyz - vertex);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float NdotH = pow(max(dot(normal, halfwayDir), 0.0), 5.0);
    // Multiply by NdotL to
    lightContribution += color.aaa * diffSpecColor.a * NdotH * NdotL;

    return lightContribution * attenuation;
}

void main(void) {
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    vec3 normal;
    // (diffuse.rgb, specular)
    vec4 diffSpecColor;
    vec4 position;

    if (MSAASamples > 1) {
        FragColor = vec4(vec3(0.0), 1.0);
        for (int i = 0; i < int(MSAASamples); ++i) {
            position = texelFetch(VertexSamplerMS, pixel, i);

#ifdef discardPixelsWithoutGeometry
            if (position.w == 0) {
                continue;
            }
#endif
            normal = texelFetch(NormalSamplerMS, pixel, i).xyz;
            diffSpecColor = texelFetch(ColorSamplerMS, pixel, i);

            for (uint i = 0; i < numLights; ++i) {
                vec3 lightContribution = calculateLightContribution(
                  i, position.xyz, normal, diffSpecColor);
                FragColor.rgb += lightContribution;
            }
        }
        FragColor.rgb /= MSAASamples;
    } else {
        position = texelFetch(VertexSampler, pixel, 0);

#ifdef discardPixelsWithoutGeometry
        // OPTIMIZATION: Don't calculate lighting for pixels that don't contain
        // any geometry
        if (position.w == 0) {
            FragColor = vec4(vec3(0.0), 1.0);
        }
#endif
        normal = texelFetch(NormalSampler, pixel, 0).xyz;
        diffSpecColor = texelFetch(ColorSampler, pixel, 0);

        FragColor = vec4(vec3(0.0), 1.0);
        for (uint i = 0; i < numLights; ++i) {
            vec3 lightContribution = calculateLightContribution(
              i, position.xyz, normal, diffSpecColor);
            FragColor.rgb += lightContribution;
        }
    }
}
