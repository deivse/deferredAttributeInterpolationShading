#version 450 core

#ifdef USER_TEST
#endif

layout(location = 0) out vec4 SphereColor;
layout(location = 1) out vec4 VertexNormal;
layout(location = 2) out vec4 VertexPosition;

layout(location = 1) uniform uint NumLights;
layout(binding = 3) uniform sampler2D AlbedoSampler;

in Vertex {
    vec3 position;
    smooth vec3 normal;
    vec2 uv;
};

void main(void) {
    SphereColor = texture(AlbedoSampler, uv);
    VertexNormal.xyz = normal;
    VertexPosition.xyz = position;
}
