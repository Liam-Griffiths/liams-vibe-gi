#version 330 core
out float FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D texNoise;

uniform vec3 samples[64];
uniform mat4 projection;

// SSAO parameters
const int kernelSize = 64;
const float radius = 0.5;
const float bias = 0.025;

void main()
{
    // Get input for SSAO algorithm
    vec3 fragPos = texture(gPosition, TexCoords).xyz;
    vec3 normal = normalize(texture(gNormal, TexCoords).rgb);
    vec3 randomVec = normalize(texture(texNoise, TexCoords * vec2(textureSize(gPosition, 0)) / 4.0).xyz);
    
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
        
        // Get sample depth
        float sampleDepth = texture(gPosition, offset.xy).z; // Get depth value of kernel sample
        
        // Range check & accumulate
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;           
    }
    occlusion = 1.0 - (occlusion / kernelSize);
    
    FragColor = occlusion;
} 