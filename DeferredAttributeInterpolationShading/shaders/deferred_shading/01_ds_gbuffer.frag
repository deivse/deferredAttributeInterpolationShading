#version 450 core

#ifdef USER_TEST
#endif

layout(location = 0) out vec4 SphereColor;
layout(location = 1) out vec4 VertexNormal;
layout(location = 2) out vec4 VertexPosition;

layout(binding = 3) uniform sampler2D AlbedoSampler;

in Vertex {
    sample vec3 position;
    sample smooth vec3 normal;
    sample vec2 uv;
};

void main(void) {
    SphereColor = texture(AlbedoSampler, uv);
    VertexNormal.xyz = normal;
#ifdef discardPixelsWithoutGeometry
    VertexPosition = vec4(position, 1);
#else
    VertexPosition.xyz = position;
#endif
}
