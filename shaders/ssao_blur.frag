#version 430 core
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
    // Decode octahedral normal for center
    vec2 cenc = texture(gNormal, TexCoords).rg;
    vec2 cf = cenc * 2.0 - 1.0;
    vec3 cn = vec3(cf.x, cf.y, 1.0 - abs(cf.x) - abs(cf.y));
    float ct = clamp(-cn.z, 0.0, 1.0);
    cn.x += (cn.x >= 0.0 ? -ct : ct);
    cn.y += (cn.y >= 0.0 ? -ct : ct);
    vec3 centerNormal = normalize(cn);
    
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
            // Decode octahedral normal for sample
            vec2 senc = texture(gNormal, sampleCoord).rg;
            vec2 sf = senc * 2.0 - 1.0;
            vec3 sn = vec3(sf.x, sf.y, 1.0 - abs(sf.x) - abs(sf.y));
            float st = clamp(-sn.z, 0.0, 1.0);
            sn.x += (sn.x >= 0.0 ? -st : st);
            sn.y += (sn.y >= 0.0 ? -st : st);
            vec3 sampleNormal = normalize(sn);
            
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