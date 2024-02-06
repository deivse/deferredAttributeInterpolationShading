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
    Triangle data[10000];
}
triangleSSB;

int get_address_from_bucket(uint id, uvec4 bucketValue) {
    if (bucketValue.x == id) return int(bucketValue.y);
    if (bucketValue.z == id) return int(bucketValue.w);
    return -1;
}

const int LOCKED = 1;
const int UNLOCKED = 0;

layout (binding = 0) uniform atomic_uint triangleSSBWriteIndex;

layout(location = 0) out uvec4 TriangleAddress;

bool lookupMemoizationCache(uint id, out int address) {
    bool store_sample = false;
    int hash = int(id) & settings.bitwiseModHashSize;
    uvec4 b = imageLoad(cache, hash);
    address = get_address_from_bucket(id, b);
    for (int k = 0; address < 0 && k < 1024; k++) {
        // ID not found in cache, make several attempts.
        uint lock = imageAtomicExchange(locks, hash, LOCKED);
        if (lock == UNLOCKED) {
            // Gain exclusive access to the bucket.
            b = imageLoad(cache, hash);
            address = get_address_from_bucket(id, b);
            if (address < 0) {
                // Allocate new storage.address
                address = int(atomicCounterIncrement(triangleSSBWriteIndex));
                b.zw = b.xy; // Update bucket FIFO.
                b.xy = uvec2(id, address);
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
            address = get_address_from_bucket(id, b);
        }
    }
    if (address < 0) { // Cache lookup failed, store redundantly.
        address = int(atomicCounterIncrement(triangleSSBWriteIndex));
        store_sample = true;
    }
    return store_sample;
}

in flat int instanceID;

void main() {
    int address = 0;
    if (lookupMemoizationCache(int(gl_PrimitiveID), address)) {
        triangleSSB.data[address].vertices[0] = vTriangleVertices[0];
        triangleSSB.data[address].vertices[1] = vTriangleVertices[1];
        triangleSSB.data[address].vertices[2] = vTriangleVertices[2];
    }

    TriangleAddress.r = address;
}
