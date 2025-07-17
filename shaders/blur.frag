#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D inputTexture;
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform int blurDirection; // 0 = horizontal, 1 = vertical

void main() {
    vec4 centerSample = texture(inputTexture, TexCoords);
    vec3 centerPos = texture(gPosition, TexCoords).xyz;
    vec3 centerNormal = normalize(texture(gNormal, TexCoords).xyz);
    
    // Early exit for background pixels
    if (length(centerNormal) < 0.1) {
        FragColor = centerSample;
        return;
    }
    
    vec3 blurredColor = vec3(0.0);
    float blurredBeta = 0.0;
    float totalWeight = 0.0;
    
    // MUCH larger kernel for aggressive GI denoising
    const float kernelRadius = 8.0; // Doubled for stronger smoothing
    const int samples = 17; // Many more samples for quality
    
    // Wider gaussian weights for stronger smoothing
    float gaussianWeights[17] = float[](
        0.012, 0.024, 0.042, 0.065, 0.082, 0.095, 0.105, 0.111, 0.113, 
        0.111, 0.105, 0.095, 0.082, 0.065, 0.042, 0.024, 0.012
    );
    
    vec2 texelSize = 1.0 / textureSize(inputTexture, 0);
    
    // Determine blur direction
    vec2 blurOffset = (blurDirection == 0) ? 
        vec2(kernelRadius * texelSize.x, 0.0) : 
        vec2(0.0, kernelRadius * texelSize.y);
    
    // Sample along one axis only (separable)
    for (int i = 0; i < samples; ++i) {
        float offsetMultiplier = float(i - samples/2);
        vec2 sampleCoord = TexCoords + blurOffset * offsetMultiplier;
        
        // Skip out-of-bounds samples
        if (sampleCoord.x < 0.0 || sampleCoord.x > 1.0 || 
            sampleCoord.y < 0.0 || sampleCoord.y > 1.0) continue;
        
        vec4 sampleData = texture(inputTexture, sampleCoord);
        vec3 samplePos = texture(gPosition, sampleCoord).xyz;
        vec3 sampleNormal = normalize(texture(gNormal, sampleCoord).xyz);
        
        // Skip background pixels
        if (length(sampleNormal) < 0.1) continue;
        
        // Much more relaxed bilateral filtering for stronger smoothing
        float depthDiff = abs(centerPos.z - samplePos.z);
        float normalDot = dot(centerNormal, sampleNormal);
        
        // Very generous weights - prioritize smoothness over detail preservation
        float depthWeight = (depthDiff < 2.0) ? 1.0 : exp(-depthDiff * 1.5); // More tolerant
        float normalWeight = max(0.3, normalDot); // More generous normal tolerance
        
        // Gaussian spatial weight
        float spatialWeight = gaussianWeights[i];
        
        // Simple combined weight - no lighting-based rejection for GI
        float finalWeight = spatialWeight * depthWeight * normalWeight;
        
        blurredColor += sampleData.rgb * finalWeight;
        blurredBeta += sampleData.a * finalWeight;
        totalWeight += finalWeight;
    }
    
    // Ensure we always have some contribution
    if (totalWeight < 0.1) {
        FragColor = centerSample;
        return;
    }
    
    // Normalize
    blurredColor /= totalWeight;
    blurredBeta /= totalWeight;
    
    // MUCH less bias towards original noisy data for GI denoising
    float originalBias = 0.15; // Was 0.6, now heavily favor the smooth result
    
    vec3 finalColor = mix(blurredColor, centerSample.rgb, originalBias);
    float finalBeta = mix(blurredBeta, centerSample.a, originalBias);
    
    // Remove energy clamping - let it smooth freely
    FragColor = vec4(finalColor, finalBeta);
} 