#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;
out vec3 ViewPos;
out vec3 ViewNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // Transform to world space
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    
    // Transform to view space for SSGI calculations
    ViewPos = vec3(view * vec4(FragPos, 1.0));
    
    // Transform normal to view space properly
    ViewNormal = mat3(transpose(inverse(view * model))) * aNormal;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
} 