#version 150
// noOp.vert.glsl:
// A fragment shader used for post-processing that simply reads the
// image produced in the first render pass by the surface shader
// and outputs it to the frame buffer


in vec2 fs_UV;

out vec4 outColor;

uniform sampler2D u_Sampler2DTexture;

void main()
{
    vec4 diffuseColor = texture(u_Sampler2DTexture, fs_UV);

    outColor = texture(u_Sampler2DTexture, fs_UV);
    //outColor = vec4(1,1,1,1);
}
