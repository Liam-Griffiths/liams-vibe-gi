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
out vec2 Velocity; // New: motion vector for TAA

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 previousView; // New: previous frame view
uniform mat4 previousProjection; // New: previous frame projection

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
    
    // Compute current NDC coordinates
    vec4 currentClip = projection * viewPos;
    vec3 currentNDC = currentClip.xyz / currentClip.w;
    
    // Compute previous NDC coordinates (assuming static model)
    vec4 previousClip = previousProjection * previousView * model * vec4(aPos, 1.0);
    vec3 previousNDC = previousClip.xyz / previousClip.w;
    
    // Velocity is difference in UV space
    Velocity = (currentNDC.xy - previousNDC.xy) * 0.5;
    
    gl_Position = currentClip;
} 