#version 450 core

#ifdef USER_TEST
#endif

layout (location = 0) out vec4 FragColor;

uniform int   u_VariableInt;
uniform float u_VariableFloat;

in vec4 v_Color;

void main(void) {
    FragColor = v_Color;
}