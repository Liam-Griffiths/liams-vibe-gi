#version 330 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedo;

in vec3 FragPos;
in vec3 Normal;
in vec3 ViewPos;
in vec3 ViewNormal;

uniform vec3 objectColor;

void main()
{    
    // Store the fragment position in view space (more suitable for SSGI)
    gPosition = ViewPos;
    
    // Store the normals in view space (already transformed in vertex shader)
    gNormal = normalize(ViewNormal);
    
    // Store diffuse per-fragment color
    gAlbedo.rgb = objectColor;
    
    // Store additional material properties (can be extended)
    gAlbedo.a = 1.0; // Currently just using alpha as a flag
} 