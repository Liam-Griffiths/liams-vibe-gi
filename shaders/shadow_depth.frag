#version 330 core

void main()
{
    // For orthographic projection, gl_FragCoord.z is linear
    gl_FragDepth = gl_FragCoord.z;
} 