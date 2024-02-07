#version 450 core

layout(early_fragment_tests) in;

struct TriangleDerivatives
{
    // Partial derivatives of each attribute for x and y
    vec2 dUV_dX;     // size = 8, offset = 0, alignment = 8
    vec2 dUV_dY;     // size = 8, offset = 8, alignment = 8
    float dW_dX;     // size = 4, offset = 16, alignment = 4
    float dW_dY;     // size = 4, offset = 20, alignment = 4
    vec3 dNormal_dX; // size = 12, offset = 32, alignment = 16
    vec3 dNormal_dY; // size = 12, offset = 48, alignment = 16

    // Attribute values shifted to (0,0) in screenspace
    vec2 UV_fixed;        // size = 8, offset = 64, alignment = 8
    vec3 normal_fixed;    // size = 12, offset = 80, alignment = 16
    float oneOverW_fixed; // size = 4, offset = 92, alignment = 4

    // ---- std430:
    //  size = 96 bytes, alignment = 16
    // --------------------------------
};
layout(std430, binding = 0) buffer TriangleDerivativesShaderStorageBuffer {
    TriangleDerivatives derivatives[];
};

layout(binding = 0) uniform isampler2D TriangleIndexSampler;

struct Light
{
    vec4 position; // (x, y, z, radius)
    vec4 color;
};

layout(binding = 0) uniform LightBuffer { Light lights[2048]; };
layout(location = 0) uniform vec3 u_CameraPosition;
layout(location = 1) uniform uint u_NumLights;
layout(binding = 3) uniform sampler2D AlbedoSampler;
uniform vec4 u_Viewport;

layout(location = 0) out vec4 FragColor;

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

void main() {
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    int index = texelFetch(TriangleIndexSampler, pixel, 0).r;

    // FragColor = vec4(texelFetch(TriangleIndexSampler, pixel, 0).rgb, 1.0);
    if (index == -1) return;

    // TODO: do ndc computation in vertex shader
    // https://stackoverflow.com/questions/49262877/opengl-ndc-and-coordinates
    vec2 viewportSize = u_Viewport.zw - u_Viewport.xy;
    vec2 ndc = (gl_FragCoord.xy - u_Viewport.xy) / viewportSize * 2.0 - 1.0;

    float oneOverW
      = (derivatives[index].oneOverW_fixed + ndc.x * derivatives[index].dW_dX
         + ndc.y * derivatives[index].dW_dY);

    vec3 normal = derivatives[index].normal_fixed
                  + ndc.x * derivatives[index].dNormal_dX
                  + ndc.y * derivatives[index].dNormal_dY;
    normal /= oneOverW;

    vec2 uv = derivatives[index].UV_fixed + ndc.x * derivatives[index].dUV_dX
              + ndc.y * derivatives[index].dUV_dY;
    uv /= oneOverW;

    vec4 diffSpecColor = texture(AlbedoSampler, uv);

    FragColor = vec4(diffSpecColor.rgb, 1.0);
    // for (uint i = 0; i < u_NumLights; ++i) {
    //     vec3 lightContribution
    //       = calculateLightContribution(i, position.xyz, normal,
    //       diffSpecColor);
    //     FragColor.rgb += lightContribution;
    // }
}
