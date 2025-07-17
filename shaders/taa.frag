#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D currentFrame;
uniform sampler2D historyFrame;
uniform sampler2D velocityBuffer;
uniform sampler2D depthBuffer;

void main() {
    vec4 current = texture(currentFrame, TexCoords);
    vec2 velocity = texture(velocityBuffer, TexCoords).rg;
    vec2 reprojectUV = TexCoords - velocity;
    
    // Check bounds
    if (reprojectUV.x < 0.0 || reprojectUV.x > 1.0 || reprojectUV.y < 0.0 || reprojectUV.y > 1.0) {
        FragColor = current;
        return;
    }
    
    vec4 history = texture(historyFrame, reprojectUV);
    
    // Depth rejection
    float currentDepth = -texture(depthBuffer, TexCoords).z; // Assuming view-space Z is negative
    float historyDepth = -texture(depthBuffer, reprojectUV).z;
    if (abs(currentDepth - historyDepth) > 0.01 * currentDepth) {
        FragColor = current;
        return;
    }
    
    // Neighborhood clamp with larger 3x3 kernel and variance
    vec3 minColor = current.rgb;
    vec3 maxColor = current.rgb;
    vec3 avgColor = vec3(0.0);
    float count = 0.0;
    vec2 texelSize = 1.0 / textureSize(currentFrame, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            vec3 neigh = texture(currentFrame, TexCoords + vec2(x, y) * texelSize).rgb;
            minColor = min(minColor, neigh);
            maxColor = max(maxColor, neigh);
            avgColor += neigh;
            count += 1.0;
        }
    }
    avgColor /= count;
    
    // Expand bounds slightly for better stability
    vec3 colorRange = maxColor - minColor;
    minColor -= colorRange * 0.1;
    maxColor += colorRange * 0.1;
    
    // Clamp history
    history.rgb = clamp(history.rgb, minColor, maxColor);
    
    // Color difference rejection
    vec3 colorDiff = abs(history.rgb - avgColor);
    float colorReject = max(colorDiff.r, max(colorDiff.g, colorDiff.b));
    float rejectFactor = smoothstep(0.05, 0.2, colorReject);
    
    // Base blend lower to reduce ghosting
    float blendFactor = 0.8; // Reduced from 0.9
    
    // More aggressive motion rejection
    float motionLength = length(velocity);
    blendFactor = mix(blendFactor, 0.1, smoothstep(0.005, 0.05, motionLength)); // More sensitive
    
    // Stronger depth rejection influence
    float depthDiff = abs(currentDepth - historyDepth);
    blendFactor *= (1.0 - smoothstep(0.0005, 0.02, depthDiff)); // Tighter threshold
    
    // Apply color rejection
    blendFactor *= (1.0 - rejectFactor * 0.8); // Strongly reduce if colors differ
    
    // Ensure minimum blend to prevent full fallback unless necessary
    blendFactor = max(blendFactor, 0.05);
    
    FragColor = mix(current, history, blendFactor);
} 