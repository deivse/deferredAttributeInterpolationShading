#version 450 core

#ifdef USER_TEST
#endif

layout (location = 0) out vec4 FragColor;

void main(void) {
    FragColor = vec4(1.0);
}