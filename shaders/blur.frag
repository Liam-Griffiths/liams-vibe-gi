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
    // Reconstruct center normal from RG16F format
    vec2 centerNormalXY = texture(gNormal, TexCoords).rg;
    float centerNormalZ = sqrt(max(0.0, 1.0 - dot(centerNormalXY, centerNormalXY)));
    vec3 centerNormal = normalize(vec3(centerNormalXY, centerNormalZ));
    
    // Early exit for background pixels
    if (length(centerNormal) < 0.1) {
        FragColor = centerSample;
        return;
    }
    
    // Universal adaptive blur strength for joining up sparse GI hits
    float centerLuminance = dot(centerSample.rgb, vec3(0.299, 0.587, 0.114));
    
    // Check if this looks like sparse GI lighting (high variance in small area)
    vec2 texelSize = 1.0 / textureSize(inputTexture, 0);
    vec3 neighbor1 = texture(inputTexture, TexCoords + vec2(texelSize.x, 0.0)).rgb;
    vec3 neighbor2 = texture(inputTexture, TexCoords + vec2(0.0, texelSize.y)).rgb;
    vec3 neighbor3 = texture(inputTexture, TexCoords + vec2(texelSize.x, texelSize.y)).rgb;
    
    float variance = length(centerSample.rgb - neighbor1) + length(centerSample.rgb - neighbor2) + length(centerSample.rgb - neighbor3);
    variance /= 3.0; // Average variance
    
    bool sparseGI = (variance > 0.1 && centerLuminance > 0.01); // High variance + some luminance = sparse GI
    
    float adaptiveBlurStrength;
    if (sparseGI) {
        adaptiveBlurStrength = 1.4; // Moderate blur for sparse GI areas (higher res data)
    } else {
        adaptiveBlurStrength = mix(1.1, 0.8, smoothstep(0.0, 0.3, centerLuminance)); // Gentler adaptive blur
    }
    
    vec3 blurredColor = vec3(0.0);
    float blurredBeta = 0.0;
    float totalWeight = 0.0;
    
    // Moderate kernel for improved dark areas without destroying resolution
    const float kernelRadius = 6.0; // Reduced for higher resolution GI data
    const int samples = 13; // Good quality without overdoing it
    
    // Balanced gaussian weights for smooth results
    float gaussianWeights[13] = float[](
        0.025, 0.045, 0.065, 0.085, 0.100, 0.110, 0.120, 
        0.110, 0.100, 0.085, 0.065, 0.045, 0.025
    );
    
    vec2 texelSize = 1.0 / textureSize(inputTexture, 0);
    
    // Determine blur direction with adaptive strength for dark areas
    vec2 blurOffset = (blurDirection == 0) ? 
        vec2(kernelRadius * texelSize.x * adaptiveBlurStrength, 0.0) : 
        vec2(0.0, kernelRadius * texelSize.y * adaptiveBlurStrength);
    
    // Sample along one axis only (separable)
    for (int i = 0; i < samples; ++i) {
        float offsetMultiplier = float(i - samples/2);
        vec2 sampleCoord = TexCoords + blurOffset * offsetMultiplier;
        
        // Skip out-of-bounds samples
        if (sampleCoord.x < 0.0 || sampleCoord.x > 1.0 || 
            sampleCoord.y < 0.0 || sampleCoord.y > 1.0) continue;
        
        vec4 sampleData = texture(inputTexture, sampleCoord);
        vec3 samplePos = texture(gPosition, sampleCoord).xyz;
                    // Reconstruct sample normal from RG16F format
            vec2 sampleNormalXY = texture(gNormal, sampleCoord).rg;
            float sampleNormalZ = sqrt(max(0.0, 1.0 - dot(sampleNormalXY, sampleNormalXY)));
            vec3 sampleNormal = normalize(vec3(sampleNormalXY, sampleNormalZ));
        
        // Skip background pixels
        if (length(sampleNormal) < 0.1) continue;
        
        // Much more relaxed bilateral filtering for stronger smoothing
        float depthDiff = abs(centerPos.z - samplePos.z);
        float normalDot = dot(centerNormal, sampleNormal);
        
        // Universal tolerance for joining up sparse GI hits
        float sampleLuminance = dot(sampleData.rgb, vec3(0.299, 0.587, 0.114));
        float avgLuminance = (centerLuminance + sampleLuminance) * 0.5;
        
        float luminanceTolerance, normalThreshold;
        if (sparseGI) {
            // Moderate permissiveness for sparse GI areas (higher res data)
            luminanceTolerance = 3.0; // Moderate depth tolerance
            normalThreshold = 0.2;    // Reasonable normal tolerance
        } else {
            // Normal adaptive tolerance
            luminanceTolerance = mix(2.0, 0.8, smoothstep(0.0, 0.4, avgLuminance));
            normalThreshold = mix(0.3, 0.6, smoothstep(0.0, 0.3, avgLuminance));
        }
        
        float depthWeight = (depthDiff < luminanceTolerance) ? 1.0 : exp(-depthDiff * 1.0);
        float normalWeight = max(normalThreshold, normalDot);
        
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
    
    // Universal bias for optimal joining of sparse GI hits (higher res data)
    float adaptiveOriginalBias;
    if (sparseGI) {
        adaptiveOriginalBias = 0.25; // Preserve more original detail with higher res data
    } else {
        adaptiveOriginalBias = mix(0.3, 0.45, smoothstep(0.0, 0.4, centerLuminance)); // Preserve more detail
    }
    
    vec3 finalColor = mix(blurredColor, centerSample.rgb, adaptiveOriginalBias);
    float finalBeta = mix(blurredBeta, centerSample.a, adaptiveOriginalBias);
    
    // Remove energy clamping - let it smooth freely
    FragColor = vec4(finalColor, finalBeta);
} 