#version 330 core
out float FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D texNoise;

uniform vec3 samples[32];
uniform mat4 projection;

// SSAO parameters tuned to avoid over-darkening
const int kernelSize = 16;
const float radius = 0.6;        // Smaller radius to limit large-scale darkening
const float bias = 0.020;        // Larger bias to reduce false occlusion
const float intensity = 1.0;     // Neutral curve (less aggressive darkening)

void main()
{
    // Get input for SSAO algorithm
    vec3 fragPos = texture(gPosition, TexCoords).xyz;
    
    // Decode octahedral normal
    vec2 enc = texture(gNormal, TexCoords).rg;
    vec2 f = enc * 2.0 - 1.0;
    vec3 n = vec3(f.x, f.y, 1.0 - abs(f.x) - abs(f.y));
    float t = clamp(-n.z, 0.0, 1.0);
    n.x += (n.x >= 0.0 ? -t : t);
    n.y += (n.y >= 0.0 ? -t : t);
    vec3 normal = normalize(n);
    
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
    occlusion = pow(occlusion, intensity);

    // Floor the AO so it never crushes the scene
    occlusion = max(occlusion, 0.5);
    
    FragColor = occlusion;
} 