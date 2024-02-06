#version 450 core

layout(early_fragment_tests) in;

struct Triangle
{
    // TODO
    vec3 vertices[3];
};

const uint TRIANGLE_SIZE = 8*4*3;

layout(binding = 2) uniform SettingsBuffer { int bitwiseModHashSize; }
settings;

layout(binding = 0, rgba32ui) coherent
  volatile restrict uniform uimageBuffer cache;
layout(binding = 1, r32ui) coherent
  volatile restrict uniform uimageBuffer locks;

// Vertices of triangle this fragment belongs to. (Passed by geometry shader)
in flat vec3 vTriangleVertices[3];

// TODO: layout, specify binding point in header
layout(std430, binding = 0) buffer TriangleShaderStorageBuffer {
    uint writeIndex;
    Triangle data[10000];
}
triangleSSB;

uint get_address_from_bucket(uint id, uvec4 bucketValue) {
    if (bucketValue.x == id) return bucketValue.y;
    if (bucketValue.z == id) return bucketValue.w;
    return -1;
}

const int LOCKED = 1;
const int UNLOCKED = 0;

layout(location = 0) out vec4 TriangleAddress;

bool lookupMemoizationCache(int id, out uint address) {
    bool store_sample = false;
    int hash = id & settings.bitwiseModHashSize;
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
                address = atomicAdd(triangleSSB.writeIndex, TRIANGLE_SIZE);
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
        address = int(atomicAdd(triangleSSB.writeIndex, TRIANGLE_SIZE));
        store_sample = true;
    }
    return store_sample;
}

void main() {
    uint address;
    if (lookupMemoizationCache(gl_PrimitiveID, address)) {
        triangleSSB.data[address].vertices[0] = vTriangleVertices[0];
        triangleSSB.data[address].vertices[1] = vTriangleVertices[1];
        triangleSSB.data[address].vertices[2] = vTriangleVertices[2];
    }

    TriangleAddress.r = address;
}
