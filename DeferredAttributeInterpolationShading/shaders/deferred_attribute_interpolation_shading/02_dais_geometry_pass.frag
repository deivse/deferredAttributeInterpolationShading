#version 450 core

layout(early_fragment_tests) in;

struct Triangle
{
    // TODO
    vec3 vertices[3];
};

layout(binding = 2) uniform SettingsBuffer {
    int bitwiseModHashSize;
    uint trianglesPerSphere;
}
settings;

layout(binding = 0, rgba32ui) coherent
  volatile restrict uniform uimageBuffer cache;
layout(binding = 1, r32ui) coherent
  volatile restrict uniform uimageBuffer locks;

// Vertices of triangle this fragment belongs to. (Passed by geometry shader)
in flat vec3 vTriangleVertices[3];
in flat uint vTriangleID;

layout(std430, binding = 0) buffer TriangleShaderStorageBuffer {
    Triangle triangles[];
};

int get_index_from_bucket(uint id, uvec4 bucketValue) {
    if (bucketValue.x == id) return int(bucketValue.y);
    if (bucketValue.z == id) return int(bucketValue.w);
    return -1;
}

const int LOCKED = 1;
const int UNLOCKED = 0;

layout (binding = 0) uniform atomic_uint triangleSSBWriteIndex;

layout(location = 0) out uvec4 TriangleIndex;

bool lookupMemoizationCache(uint id, out int index) {
    bool store_sample = false;
    int hash = int(id) & settings.bitwiseModHashSize;
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

in flat int instanceID;

void main() {
    int index = 0;
    if (lookupMemoizationCache(int(gl_PrimitiveID), index)) {
        triangles[index].vertices[0] = vTriangleVertices[0];
        triangles[index].vertices[1] = vTriangleVertices[1];
        triangles[index].vertices[2] = vTriangleVertices[2];
    }

    TriangleIndex.r = index;
}
