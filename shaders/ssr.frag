#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;  
uniform sampler2D gAlbedo;
uniform sampler2D colorTexture; // Current frame color for reflection sampling
uniform mat4 view;
uniform mat4 projection;
uniform vec3 viewPos;

// SSR Parameters
const int MAX_STEPS = 64;
const int BINARY_SEARCH_STEPS = 8;
const float RAY_STEP = 0.04;
const float MIN_RAY_STEP = 0.01;
const float MAX_DISTANCE = 50.0;
const float THICKNESS = 0.1;

vec3 SSRTrace(vec3 rayOrigin, vec3 rayDirection, out bool hitFound) {
    hitFound = false;
    
    // Transform ray to view space for screen-space raymarching
    vec4 rayEnd = vec4(rayOrigin + rayDirection * MAX_DISTANCE, 1.0);
    vec4 rayEndClip = projection * rayEnd;
    vec3 rayEndScreen = rayEndClip.xyz / rayEndClip.w;
    rayEndScreen = rayEndScreen * 0.5 + 0.5;
    
    vec4 rayStartClip = projection * vec4(rayOrigin, 1.0);
    vec3 rayStartScreen = rayStartClip.xyz / rayStartClip.w;
    rayStartScreen = rayStartScreen * 0.5 + 0.5;
    
    vec3 rayDelta = rayEndScreen - rayStartScreen;
    float rayLength = length(rayDelta.xy);
    rayDelta = normalize(rayDelta);
    
    // Adaptive step size based on distance
    float stepSize = RAY_STEP;
    if (rayLength < 0.1) stepSize = MIN_RAY_STEP;
    
    vec3 currentPos = rayStartScreen;
    
    // Raymarching loop
    for (int i = 0; i < MAX_STEPS; ++i) {
        currentPos += rayDelta * stepSize;
        
        // Check bounds
        if (currentPos.x < 0.0 || currentPos.x > 1.0 || 
            currentPos.y < 0.0 || currentPos.y > 1.0) {
            break;
        }
        
        // Sample depth at current position
        float sceneDepth = texture(gPosition, currentPos.xy).z;
        float rayDepth = currentPos.z;
        
        // Check for intersection
        if (rayDepth > sceneDepth && rayDepth - sceneDepth < THICKNESS) {
            // Binary search refinement for accuracy
            vec3 refinedPos = currentPos;
            vec3 searchStep = rayDelta * stepSize * 0.5;
            
            for (int j = 0; j < BINARY_SEARCH_STEPS; ++j) {
                float refineDepth = texture(gPosition, refinedPos.xy).z;
                if (refinedPos.z > refineDepth) {
                    refinedPos -= searchStep;
                } else {
                    refinedPos += searchStep;
                }
                searchStep *= 0.5;
            }
            
            hitFound = true;
            return texture(colorTexture, refinedPos.xy).rgb;
        }
        
        // Adaptive step size increase with distance
        stepSize = min(stepSize * 1.05, RAY_STEP * 2.0);
    }
    
    return vec3(0.0); // No hit found
}

void main() {
    // Sample G-buffer data
    vec3 viewPos = texture(gPosition, TexCoords).xyz;
    vec2 normalXY = texture(gNormal, TexCoords).rg;
    float normalZ = sqrt(max(0.0, 1.0 - dot(normalXY, normalXY)));
    vec3 normal = normalize(vec3(normalXY, normalZ));
    vec4 albedoRoughness = texture(gAlbedo, TexCoords);
    float roughness = albedoRoughness.a;
    
    // Skip reflection for very rough surfaces
    if (roughness > 0.8) {
        FragColor = vec4(0.0, 0.0, 0.0, 0.0);
        return;
    }
    
    // Calculate reflection ray
    vec3 viewDir = normalize(viewPos); // In view space, camera is at origin
    vec3 reflectDir = reflect(viewDir, normal);
    
    // Perform SSR trace
    bool hitFound;
    vec3 reflectionColor = SSRTrace(viewPos, reflectDir, hitFound);
    
    if (hitFound) {
        // Calculate reflection strength based on surface properties
        float fresnel = pow(1.0 - max(dot(-viewDir, normal), 0.0), 2.0);
        float roughnessAttenuation = 1.0 - roughness;
        float reflectionStrength = fresnel * roughnessAttenuation;
        
        // Distance attenuation based on ray length
        float distanceAttenuation = 1.0; // Could add distance-based falloff here
        
        reflectionStrength *= distanceAttenuation;
        
        FragColor = vec4(reflectionColor * reflectionStrength, reflectionStrength);
    } else {
        FragColor = vec4(0.0, 0.0, 0.0, 0.0);
    }
} 