#version 150

in vec4 fs_Col;

out vec4 out_Col; // This is the final output color that you will see on your
// screen for the pixel that is currently being processed.

void main()
{

    out_Col = fs_Col;
}
