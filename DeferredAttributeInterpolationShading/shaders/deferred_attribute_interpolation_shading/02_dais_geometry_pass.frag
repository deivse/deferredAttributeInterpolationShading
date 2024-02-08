#version 450 core
#extension GL_ARB_post_depth_coverage : require

layout(post_depth_coverage) in;
layout(early_fragment_tests) in;

struct Triangle
{
    // in std430, vec3s are not padded to vec4 when in an array or structure.
    vec4 vertices[3];        // size = 48, offset = 0, alignment = 16
    uint normalsSnormOct[3]; // size = 12, offset = 48, alignment = 4
    uint UVsUnorm[3];        // size = 12, offset = 60, alignment = 4

    // 96 - 72 = 24 bytes padding
    uint padding[6]; // size = 24, offset = 72, alignment = 4

    // ---- std430:
    // size == 96 bytes, alignment = 16
    // --------------------------------
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

layout(binding = 0, rgba32ui) coherent
  volatile restrict uniform uimageBuffer cache;
layout(binding = 1, r32ui) coherent
  volatile restrict uniform uimageBuffer locks;

// Vertices of triangle this fragment belongs to. (Passed by geometry shader)
in flat vec4 vVertices[3];
in flat uint vNormalsSnormOct[3];
in flat uint vUVsSnorm[3];

layout(std430, binding = 0) buffer TriangleShaderStorageBuffer {
    Triangle triangles[];
};

layout(binding = 0) uniform atomic_uint triangleSSBWriteIndex;

int get_index_from_bucket(uint id, uvec4 bucketValue) {
    if (bucketValue.x == id) return int(bucketValue.y);
    if (bucketValue.z == id) return int(bucketValue.w);
    return -1;
}

const int LOCKED = 1;
const int UNLOCKED = 0;

bool lookupMemoizationCache(uint id, out int index) {
    bool store_sample = false;
    int hash = int(id) & bitwiseModHashSize;
    uvec4 b = imageLoad(cache, hash);
    index = get_index_from_bucket(id, b);
    for (int k = 0; index < 0 && k < 1024; k++) {
        // ID not found in cache, make several attempts.
        uint lock = imageAtomicExchange(locks, hash, LOCKED);
        if (lock == UNLOCKED) {
            // Gain exclusive access to the bucket.
            b = imageLoad(cache, hash);
            index = get_index_from_bucket(id, b);
            if (index < 0) {
                // Allocate new storage.index
                index = int(atomicCounterIncrement(triangleSSBWriteIndex));
                b.zw = b.xy; // Update bucket FIFO.
                b.xy = uvec2(id, index);
                imageStore(cache, hash, b);
                store_sample = true;
            }
            imageStore(locks, hash, uvec4(UNLOCKED));
        }
        // Use if (expr){} if (!expr) {} construct to explicitly sequence the
        // branches
        if (lock == LOCKED) {
            for (int i = 0; i < 128 && lock == LOCKED; i++) {
                lock = imageLoad(locks, hash).r;
            }
            b = imageLoad(cache, hash);
            index = get_index_from_bucket(id, b);
        }
    }
    if (index < 0) { // Cache lookup failed, store redundantly.
        index = int(atomicCounterIncrement(triangleSSBWriteIndex));
        store_sample = true;
    }
    return store_sample;
}

layout(location = 0) out uvec4 TriangleIndex;

void main() {
    int index = 0;
    if (lookupMemoizationCache(int(gl_PrimitiveID), index)) {
        triangles[index].vertices[0] = vVertices[0];
        triangles[index].vertices[1] = vVertices[1];
        triangles[index].vertices[2] = vVertices[2];
        triangles[index].normalsSnormOct[0] = vNormalsSnormOct[0];
        triangles[index].normalsSnormOct[1] = vNormalsSnormOct[1];
        triangles[index].normalsSnormOct[2] = vNormalsSnormOct[2];
        triangles[index].UVsUnorm[0] = vUVsSnorm[0];
        triangles[index].UVsUnorm[1] = vUVsSnorm[1];
        triangles[index].UVsUnorm[2] = vUVsSnorm[2];
    }

    int coverage = gl_SampleMaskIn[gl_SampleID];
    TriangleIndex.r = index;
    // TriangleIndex.r |= coverage;
}
