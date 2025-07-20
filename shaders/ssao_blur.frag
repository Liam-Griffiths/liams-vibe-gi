#version 330 core
out float FragColor;

in vec2 TexCoords;

uniform sampler2D ssaoInput;
uniform sampler2D gPosition;
uniform sampler2D gNormal;

void main() 
{
    vec2 texelSize = 1.0 / vec2(textureSize(ssaoInput, 0));
    float result = 0.0;
    float totalWeight = 0.0;
    
    // Get center values for bilateral filtering
    float centerOcclusion = texture(ssaoInput, TexCoords).r;
    vec3 centerPosition = texture(gPosition, TexCoords).xyz;
    vec2 centerNormalXY = texture(gNormal, TexCoords).rg;
    float centerNormalZ = sqrt(max(0.0, 1.0 - dot(centerNormalXY, centerNormalXY)));
    vec3 centerNormal = normalize(vec3(centerNormalXY, centerNormalZ));
    
    // Bilateral blur with edge preservation
    for (int x = -2; x <= 2; ++x) 
    {
        for (int y = -2; y <= 2; ++y) 
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            vec2 sampleCoord = TexCoords + offset;
            
            // Skip out-of-bounds samples
            if (sampleCoord.x < 0.0 || sampleCoord.x > 1.0 || 
                sampleCoord.y < 0.0 || sampleCoord.y > 1.0) {
                continue;
            }
            
            float sampleOcclusion = texture(ssaoInput, sampleCoord).r;
            vec3 samplePosition = texture(gPosition, sampleCoord).xyz;
            vec2 sampleNormalXY = texture(gNormal, sampleCoord).rg;
            float sampleNormalZ = sqrt(max(0.0, 1.0 - dot(sampleNormalXY, sampleNormalXY)));
            vec3 sampleNormal = normalize(vec3(sampleNormalXY, sampleNormalZ));
            
            // Calculate weights based on depth and normal similarity
            float depthWeight = 1.0 / (1.0 + abs(centerPosition.z - samplePosition.z) * 10.0);
            float normalWeight = max(0.1, dot(centerNormal, sampleNormal));
            
            // Distance weight (gaussian-like)
            float distanceWeight = exp(-dot(offset, offset) * 200.0);
            
            // Combined weight
            float weight = depthWeight * normalWeight * distanceWeight;
            
            result += sampleOcclusion * weight;
            totalWeight += weight;
        }
    }
    
    // Normalize result
    if (totalWeight > 0.0) {
        result /= totalWeight;
    } else {
        result = centerOcclusion; // Fallback to original if no valid samples
    }
    
    FragColor = result;
} 