#version 450 core
layout (triangles, invocations = 1) in;
layout (triangle_strip, max_vertices = 3) out;

in gl_PerVertex
{
    vec4 gl_Position;
} gl_in[];

out gl_PerVertex
{
    vec4 gl_Position;
};

in flat int instanceID[3];
out flat vec3 vTriangleVertices [3];

void main()
{
    for (int i = 0; i < gl_in.length(); ++i)
    {
        gl_Position = gl_in[i].gl_Position;
        vTriangleVertices[i] = gl_in[i].gl_Position.xyz;
        EmitVertex();
    }
    gl_PrimitiveID = gl_PrimitiveIDIn;
    EndPrimitive();
}
