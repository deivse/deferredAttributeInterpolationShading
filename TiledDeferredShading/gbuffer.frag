#version 450 core

layout (location = 0) out vec4 Color;  // Multiple render targets
layout (location = 1) out vec4 Normal;
layout (location = 2) out vec4 Vertex;

in vec2 v_TexCoord;
in vec3 v_Normal;
in vec4 v_Vertex;

layout (binding = 0)  uniform sampler2D u_Texture;

void main(void) {
    Color  = texture(u_Texture, v_TexCoord); // (diffuse.rgb, specular)
    Normal = vec4(normalize(v_Normal), gl_FragCoord.z); // Save fragment’s depth for later usage
    Vertex = v_Vertex;
}
