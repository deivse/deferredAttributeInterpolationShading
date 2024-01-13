#version 450 core

layout (location = 0) out vec4 FragColor;

layout (binding = 1) uniform sampler2D Color;
layout (binding = 2) uniform sampler2D Normal;
layout (binding = 3) uniform sampler2D Vertex;

layout (location = 1) uniform vec3 u_CameraPosition;
layout (location = 2) uniform uint u_NumLights;

struct Light {
    vec4 position; // (x, y, z, radius) 
    vec4 color;    // (diffuse.rgb, specular)
};
layout (binding = 0) uniform LightBuffer {
    Light light[2048];
};

#ifdef Tiled_Shading
    // Tiled Deferred Shading declations 
    const uint MaxLights = 2048;
    layout (binding = 0) buffer LightCounter {
        uint lightCount[];
    };
    layout (binding = 1) buffer LightID {
        uint lightID[];
    };
    layout (location = 4) uniform uint u_ResolutionX;
    layout (location = 5) uniform uint u_TileSize;
#endif

// Simple phong lighting
vec3 lighting(in uint lightIdx, in vec3 vertex, in vec3 normal, in vec4 diffSpecColor) {
    vec4 lightPos = light[lightIdx].position;

    // Check if point is out of the light's range
    vec3 lightDir   = lightPos.xyz - vertex;
    float distance  = length(lightDir);
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
    ivec2 pixelCoord = ivec2(gl_FragCoord.xy);
    vec4 normal        = texelFetch(Normal, pixelCoord, 0);
    vec4 diffSpecColor = texelFetch( Color, pixelCoord, 0); // (diffuse.rgb, specular)
    vec4 v_Vertex      = texelFetch(Vertex, pixelCoord, 0);

    // sum lighting
    vec3 color = vec3(0.0);
    if (v_Vertex.w > 0.0) {
#ifdef Tiled_Shading
        // Sum lighting from all light sources affecting the fragment's tile
        uvec2 tileCoord = pixelCoord / u_TileSize;
        uint tileIndex = tileCoord.y * u_ResolutionX + tileCoord.x;
        uint numTileLights = lightCount[tileIndex];
        uint base = tileIndex * MaxLights;
        for (uint i = 0; i < numTileLights; i++) {
            color += lighting(lightID[base + i], v_Vertex.xyz, normal.xyz, diffSpecColor);
        }
#else
        // Sum lighting from all light sources
        for (uint i = 0; i < u_NumLights; i++) {
            color += lighting(i, v_Vertex.xyz, normal.xyz, diffSpecColor);
        }
#endif
    }

#ifndef Restore_Depth
    gl_FragDepth = (v_Vertex.w > 0.0) ? normal.w : 1.0;
#endif

    FragColor = vec4(color, 1.0);
}
