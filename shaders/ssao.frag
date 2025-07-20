#version 330 core
out float FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D texNoise;

uniform vec3 samples[32];
uniform mat4 projection;

// Optimized SSAO parameters for better performance/quality balance
const int kernelSize = 16;       // Reduced from 32 to 16 for ~2x performance improvement
const float radius = 0.8;        // Slightly reduced radius for better locality
const float bias = 0.008;        // Adjusted bias for fewer samples
const float intensity = 1.8;     // Increased intensity to compensate for fewer samples

void main()
{
    // Get input for SSAO algorithm
    vec3 fragPos = texture(gPosition, TexCoords).xyz;
    
    // Reconstruct normal from RG16F format
    vec2 normalXY = texture(gNormal, TexCoords).rg;
    float normalZ = sqrt(max(0.0, 1.0 - dot(normalXY, normalXY)));
    vec3 normal = normalize(vec3(normalXY, normalZ));
    
    // Early exit optimizations for better performance
    // Skip SSAO for very far objects to improve performance
    if (length(fragPos) > 30.0) {
        FragColor = 1.0;
        return;
    }
    
    // Skip SSAO for background pixels (no geometry)
    if (length(normal) < 0.1) {
        FragColor = 1.0;
        return;
    }
    
    // Enhanced noise sampling with better tiling
    vec2 noiseScale = vec2(textureSize(gPosition, 0)) / 4.0;
    vec3 randomVec = normalize(texture(texNoise, TexCoords * noiseScale).xyz);
    
    // Create TBN matrix to transform samples to view space
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    
    // Iterate over the sample kernel and calculate occlusion factor
    float occlusion = 0.0;
    for(int i = 0; i < kernelSize; ++i)
    {
        // Get sample position
        vec3 samplePos = TBN * samples[i]; // From tangent to view-space
        samplePos = fragPos + samplePos * radius; 
        
        // Project sample position to get screen-space position
        vec4 offset = vec4(samplePos, 1.0);
        offset = projection * offset; // View to clip-space
        offset.xyz /= offset.w; // Perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5; // Transform to range 0.0 - 1.0
        
        // Skip samples outside screen space
        if (offset.x < 0.0 || offset.x > 1.0 || offset.y < 0.0 || offset.y > 1.0) {
            continue;
        }
        
        // Get sample depth
        float sampleDepth = texture(gPosition, offset.xy).z;
        
        // Enhanced range check with smoother falloff
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        
        // Improved occlusion test with better bias handling
        float depthDifference = sampleDepth - samplePos.z;
        float occlusionFactor = (depthDifference >= bias) ? 1.0 : 0.0;
        
        // Apply range check and accumulate
        occlusion += occlusionFactor * rangeCheck;           
    }
    
    // Normalize and apply intensity
    occlusion = 1.0 - (occlusion / kernelSize);
    occlusion = pow(occlusion, intensity); // Apply intensity curve
    
    // Ensure we don't go completely black
    occlusion = max(occlusion, 0.1);
    
    FragColor = occlusion;
} 