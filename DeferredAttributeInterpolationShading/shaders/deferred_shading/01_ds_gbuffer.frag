#version 450 core

#ifdef USER_TEST
#endif

#ifdef StoreCoverage
    #extension GL_ARB_post_depth_coverage : require

layout(post_depth_coverage) in;
layout(early_fragment_tests) in;
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
    VertexPosition = vec4(position, 1);

#ifdef StoreCoverage
    if (gl_NumSamples > 1) {
        VertexPosition.w = uintBitsToFloat(gl_SampleMaskIn[0]);
    }
#endif
}
