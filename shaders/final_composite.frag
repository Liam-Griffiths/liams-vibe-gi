#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D ssgiTexture;
uniform sampler2D shadowMap;

// Lighting uniforms
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform mat4 lightSpaceMatrix;
uniform mat4 view;

// SSGI parameters
uniform float ssgiStrength;
uniform float ambientStrength;

// Shadow calculation (same as lighting.frag)
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    float currentDepth = projCoords.z;
    
    float bias = max(0.01 * (1.0 - dot(normal, lightDir)), 0.001);
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    if(projCoords.z > 1.0)
        shadow = 0.0;
        
    return shadow;
}

void main()
{
    vec3 position = texture(gPosition, TexCoords).xyz;
    vec3 normal = texture(gNormal, TexCoords).xyz;
    vec3 albedo = texture(gAlbedo, TexCoords).rgb;
    vec3 ssgi = texture(ssgiTexture, TexCoords).rgb;
    
    // Skip background pixels
    if (length(normal) < 0.1) {
        FragColor = vec4(0.1, 0.1, 0.1, 1.0); // Dark background
        return;
    }
    
    // Convert view space position back to world space for lighting calculations
    vec4 worldPos = inverse(view) * vec4(position, 1.0);
    vec3 fragPos = worldPos.xyz;
    vec3 worldNormal = normalize(mat3(inverse(view)) * normal);
    
    // Calculate light space position for shadows
    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(fragPos, 1.0);
    
    // Basic lighting calculations
    vec3 ambient = ambientStrength * lightColor;
    
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(worldNormal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, worldNormal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = 0.5 * spec * lightColor;
    
    // Calculate shadow
    float shadow = ShadowCalculation(fragPosLightSpace, worldNormal, lightDir);
    
    // Combine direct lighting
    vec3 directLighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * albedo;
    
    // Add SSGI contribution
    vec3 indirectLighting = ssgi * albedo * ssgiStrength;
    
    // Final color combination
    vec3 finalColor = directLighting + indirectLighting;
    
    // Tone mapping and gamma correction
    finalColor = finalColor / (finalColor + vec3(1.0));
    finalColor = pow(finalColor, vec3(1.0/2.2));
    
    FragColor = vec4(finalColor, 1.0);
} 