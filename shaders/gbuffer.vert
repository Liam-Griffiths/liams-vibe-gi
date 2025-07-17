#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out vec3 FragPos;
out vec3 Normal;
out vec3 ViewPos;
out vec3 ViewNormal;
out vec2 TexCoords;
out vec3 ViewTangent;
out vec3 ViewBitangent;
out vec2 Velocity; // Motion vector (simplified, no jittering)

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 previousView;
uniform mat4 previousProjection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    
    // Compute view-space positions
    vec4 viewPos = view * vec4(FragPos, 1.0);
    ViewPos = viewPos.xyz;
    ViewNormal = mat3(transpose(inverse(view * model))) * aNormal;
    
    // Pass texture coordinates and compute view-space tangent vectors
    TexCoords = aTexCoords;
    ViewTangent = mat3(transpose(inverse(view * model))) * aTangent;
    ViewBitangent = mat3(transpose(inverse(view * model))) * aBitangent;
    
    // Simplified motion vectors for effects that might still need them
    vec4 currentClip = projection * viewPos;
    vec4 previousViewPos = previousView * vec4(FragPos, 1.0);
    vec4 previousClip = previousProjection * previousViewPos;
    
    vec2 currentScreen = (currentClip.xy / currentClip.w) * 0.5 + 0.5;
    vec2 previousScreen = (previousClip.xy / previousClip.w) * 0.5 + 0.5;
    
    // Simple, clean motion vector
    Velocity = currentScreen - previousScreen;
    
    gl_Position = currentClip;
} 