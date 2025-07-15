#version 330 core
out vec3 FragColor;

in vec2 TexCoords;

uniform sampler2D ssgiTexture;
uniform sampler2D gNormal;
uniform sampler2D gPosition;
uniform vec2 screenSize;
uniform bool horizontal;

void main()
{
    vec2 texelSize = 1.0 / screenSize;
    vec3 result = texture(ssgiTexture, TexCoords).rgb;
    
    vec3 centerNormal = texture(gNormal, TexCoords).xyz;
    vec3 centerPosition = texture(gPosition, TexCoords).xyz;
    
    // Skip background pixels
    if (length(centerNormal) < 0.1) {
        FragColor = result;
        return;
    }
    
    float totalWeight = 1.0;
    
    // 5-tap blur with normal and depth-aware filtering
    for (int i = 1; i <= 2; ++i) {
        vec2 offset = horizontal ? vec2(texelSize.x * float(i), 0.0) : vec2(0.0, texelSize.y * float(i));
        
        // Sample both directions
        for (int dir = -1; dir <= 1; dir += 2) {
            vec2 sampleCoord = TexCoords + offset * float(dir);
            
            if (sampleCoord.x >= 0.0 && sampleCoord.x <= 1.0 && 
                sampleCoord.y >= 0.0 && sampleCoord.y <= 1.0) {
                
                vec3 sampleNormal = texture(gNormal, sampleCoord).xyz;
                vec3 samplePosition = texture(gPosition, sampleCoord).xyz;
                vec3 sampleColor = texture(ssgiTexture, sampleCoord).rgb;
                
                // Calculate weights based on normal and depth similarity
                float normalWeight = max(0.0, dot(centerNormal, sampleNormal));
                normalWeight = pow(normalWeight, 4.0); // Sharp falloff
                
                float depthWeight = 1.0 / (1.0 + abs(centerPosition.z - samplePosition.z) * 10.0);
                
                float weight = normalWeight * depthWeight;
                
                result += sampleColor * weight;
                totalWeight += weight;
            }
        }
    }
    
    FragColor = result / totalWeight;
} 