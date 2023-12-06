#version 450 core

void main(void) {
    vec2 xy = vec2((gl_VertexID & 1) << 2,(gl_VertexID & 2) << 1) - vec2(1.0);
    gl_Position = vec4(xy, 0.0, 1.0);
}
