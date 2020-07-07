#version 150
// passthrough.vert.glsl:
// A vertex shader that simply passes along vertex data
// to the fragment shader without operating on it in any way.

in vec4 vs_Pos;
in vec2 vs_UV;

out vec2 fs_UV;


uniform ivec2 u_Dimensions; // Screen dimensions


void main()
{
    fs_UV = vs_UV;

    gl_Position = vec4(vs_Pos.x, vs_Pos.y, vs_Pos.z, vs_Pos.w);
}
