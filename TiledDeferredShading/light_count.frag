#version 450 core

layout (binding = 0) buffer LightCounter {
    uint lightCount[];
};
layout (binding = 1) buffer LightID {
    uint lightID[];
};
layout (location = 0) uniform uint u_ResolutionX;

flat in uint v_LightID;

const uint MaxLights = 2048; // TODO: g_NumLights should be sufficient

void main(void) {
    ivec2 tileCoord = ivec2(gl_FragCoord.xy);
    uint  tileIndex = tileCoord.y * u_ResolutionX + tileCoord.x;
    
    // Increment number of lights affecting the tile. Atomic increment must be used.
    uint numTileLights = atomicAdd(lightCount[tileIndex], 1);

    // Base offset into `lightID` buffer
    uint baseOffset = tileIndex * MaxLights;
    
    // Save ID of the light into the buffer. There is no need to use atomic
    // operation because fragment shader has unique offset value (numTileLights).
    lightID[baseOffset + numTileLights] = v_LightID;
}