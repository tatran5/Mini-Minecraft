#version 150
// ^ Change this to version 130 if you have compatibility issues

// Refer to the lambert shader files for useful comments

// This vertex shader takes a per-object model,
// a vertex and transforms all vertices to light space using lightSpaceMatrix
in vec4 vs_Pos; // x

uniform mat4 u_LightSpaceMatrix; // x
uniform mat4 u_Model; // x // The matrix that defines the transformation of the
// object we're rendering. In this assignment,
// this will be the result of traversing your scene graph.

void main()
{
    gl_Position = u_LightSpaceMatrix * u_Model * vs_Pos;
}
