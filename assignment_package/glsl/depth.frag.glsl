#version 150
// ^ Change this to version 130 if you have compatibility issues

// Refer to the lambert shader files for useful comments

// Since we have no color buffer the resulting fragments do not require any processing,
// so we can simply use an empty fragment shader:
void main()
{
    // gl_FragDepth = gl_FragCoord.z;
}
