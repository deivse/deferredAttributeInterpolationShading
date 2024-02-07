#version 450 core

layout(location = 0) in vec4 a_Vertex;
layout(binding = 1) uniform SphereCentersBuffer { vec4 sphereOffsets[2048]; };

uniform mat4 u_MVPMatrix;

out int vInstanceID;

out uint vNormalSnormOct;
out uint vUVsnorm;

// Returns ±1
vec2 signNotZero(vec2 v) {
    return vec2((v.x >= 0.0) ? +1.0 : -1.0, (v.y >= 0.0) ? +1.0 : -1.0);
}
// Assume normalized input. Output is on [-1, 1] for each component.
vec2 float32x3_to_oct(in vec3 v) {
    // Project the sphere onto the octahedron, and then onto the xy plane
    vec2 p = v.xy * (1.0 / (abs(v.x) + abs(v.y) + abs(v.z)));
    // Reflect the folds of the lower hemisphere over the diagonals
    return (v.z <= 0.0) ? ((1.0 - abs(p.yx)) * signNotZero(p)) : p;
}

void main(void) {
    gl_Position = u_MVPMatrix
                  * vec4(a_Vertex.xyz + sphereOffsets[gl_InstanceID].xyz, 1.0);
    vNormalSnormOct = packSnorm2x16(float32x3_to_oct(a_Vertex.xyz));
    vUVsnorm = packSnorm2x16(a_Vertex.xy);

    // Used in geometry shader to calculate globally unique triangle id
    vInstanceID = gl_InstanceID;
}
