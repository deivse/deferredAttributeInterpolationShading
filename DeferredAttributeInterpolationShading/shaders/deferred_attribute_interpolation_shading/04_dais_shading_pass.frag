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

struct Light
{
    vec4 position; // (x, y, z, radius)
    vec4 color;
};

layout(std430, binding = 0) buffer TriangleDerivativesShaderStorageBuffer {
    TriangleDerivatives derivatives[];
};
layout(std430, binding = 2) buffer LightBuffer {
    uint numLights; // size = 4, offset = 0
    Light lights[]; // size = 32, offset = 16, alignment = 16
};

layout(std140, binding = 1) uniform DAISUniforms {
    vec4 cameraPosition;       // size = 16, offset = 0, alignment = 16
    mat4 MVPMatrix;            // size = 64, offset = 16, alignment = 16
    mat4 MVPMatrixInv;         // size = 64, offset = 80, alignment = 16
    vec4 Viewport;             // size = 16, offset = 144, alignment = 16
    int bitwiseModHashSize;    // size = 4, offset = 160, alignment = 4
    uint trianglesPerSphere;   // size = 4, offset = 164, alignment = 4
    float projectionMatrix_32; // size = 4, offset = 168, alignment = 4
    float projectionMatrix_22; // size = 4, offset = 172, alignment = 4
    uint numSamples;           // size = 4, offset = 176, alignment = 4

    // ---- std140:
    // size = 180, alignment = 16
    // -------------------------
};

layout(binding = 0) uniform isampler2D TriangleIndexSampler;
layout(binding = 4) uniform isampler2DMS TriangleAddressMultiSampler;
layout(binding = 3) uniform sampler2D AlbedoSampler;

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
    vec3 viewDir = normalize(cameraPosition.xyz - vertex);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float NdotH = pow(max(dot(normal, halfwayDir), 0.0), 5.0);
    // Multiply by NdotL to
    lightContribution += color.aaa * diffSpecColor.a * NdotH * NdotL;

    return lightContribution * attenuation;
}

vec3 shadePixel(int index) {
    vec2 viewportSize = Viewport.zw - Viewport.xy;
    vec4 ndcPos;
    ndcPos.xy = (gl_FragCoord.xy - Viewport.xy) / viewportSize * 2.0 - 1.0; 

    float oneOverW
      = (derivatives[index].oneOverW_fixed + ndcPos.x * derivatives[index].dW_dX
         + ndcPos.y * derivatives[index].dW_dY);

    ndcPos.z = projectionMatrix_32 * oneOverW - projectionMatrix_22;
    ndcPos.w = 1;

    vec4 clipPos = ndcPos / oneOverW;
    vec4 worldPos = MVPMatrixInv * clipPos;

    vec3 normal = (derivatives[index].normal_fixed //
                   + ndcPos.x * derivatives[index].dNormal_dX
                   + ndcPos.y * derivatives[index].dNormal_dY)
                  / oneOverW;

    vec2 uv = (derivatives[index].UV_fixed //
               + ndcPos.x * derivatives[index].dUV_dX
               + ndcPos.y * derivatives[index].dUV_dY)
              / oneOverW;

    vec4 diffSpecColor = texture(AlbedoSampler, uv);
    vec3 retval = vec3(0);
    for (uint i = 0; i < numLights; ++i) {
        vec3 lightContribution
          = calculateLightContribution(i, worldPos.xyz, normal, diffSpecColor);
        retval += lightContribution;
    }
    return retval;
}


// TODO: image brighter at 8X MSAA??
vec3 shadeMultisample() {
    // mask stores which samples need to be shaded
    uint mask = (1 << numSamples) - 1;
    vec3 accum = vec3(0.0);
    while (mask > 0) {
        int i = findLSB(mask); // next sample index to shade
        int vs = texelFetch(TriangleAddressMultiSampler,
                                   ivec2(gl_FragCoord.xy), i).r;
        if (vs != -1) {                  // is there a triangle referenced?
            uint sample_mask = vs >> 24; // extract coverage
            int t = vs & 0x00ffffff;    // extract triangle idx
            accum += bitCount(sample_mask) * shadePixel(t);
            mask &= ~sample_mask; // mark shaded samples
        } else {                  // no triangle referenced
            accum += vec3(0.0); // accumulate background color
            mask &= ~(1 << i); // mark shaded sample
        }
    }
    return accum / float(numSamples);
}

void main() {
    if (numSamples > 0) {
        FragColor = vec4(shadeMultisample(), 1.0); 
    } else {
        int index
          = texelFetch(TriangleIndexSampler, ivec2(gl_FragCoord.xy), 0).r;
        if (index == -1) discard;
        FragColor = vec4(shadePixel(index & 0x00ffffff), 1.0);
    }
}
