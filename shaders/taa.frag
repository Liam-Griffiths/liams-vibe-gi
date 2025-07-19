#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D currentFrame;
uniform sampler2D historyFrame;
uniform sampler2D gVelocity;
uniform sampler2D gPosition;
uniform mat4 currentViewProj;
uniform mat4 previousViewProj;
uniform float frameCounter;

// TAA Parameters - Much more aggressive to eliminate ghosting
const float VELOCITY_WEIGHT = 0.02;
const float MIN_BLEND_FACTOR = 0.15;  // Much more responsive to changes
const float MAX_BLEND_FACTOR = 0.50;  // Allow much more current frame contribution
const float LUMA_WEIGHT = 0.15;

vec3 RGB2YCoCg(vec3 RGB) {
    float Y  = dot(RGB, vec3( 0.25, 0.5,  0.25));
    float Co = dot(RGB, vec3( 0.5,  0.0, -0.5 ));
    float Cg = dot(RGB, vec3(-0.25, 0.5, -0.25));
    return vec3(Y, Co, Cg);
}

vec3 YCoCg2RGB(vec3 YCoCg) {
    float Y  = YCoCg.x;
    float Co = YCoCg.y;
    float Cg = YCoCg.z;
    return vec3(Y + Co - Cg, Y + Cg, Y - Co - Cg);
}

// Catmull-Rom sampling for better quality
vec3 sampleCatmullRom(sampler2D tex, vec2 uv, vec2 texSize) {
    vec2 samplePos = uv * texSize;
    vec2 texPos1 = floor(samplePos - 0.5) + 0.5;
    vec2 f = samplePos - texPos1;
    vec2 w0 = f * (-0.5 + f * (1.0 - 0.5 * f));
    vec2 w1 = 1.0 + f * f * (-2.5 + 1.5 * f);
    vec2 w2 = f * (0.5 + f * (2.0 - 1.5 * f));
    vec2 w3 = f * f * (-0.5 + 0.5 * f);
    
    vec2 w12 = w1 + w2;
    vec2 offset12 = w2 / (w1 + w2);
    
    vec2 texPos0 = texPos1 - vec2(1.0);
    vec2 texPos3 = texPos1 + vec2(2.0);
    vec2 texPos12 = texPos1 + offset12;
    
    texPos0 /= texSize;
    texPos3 /= texSize;
    texPos12 /= texSize;
    
    vec3 result = texture(tex, texPos0).rgb * (w0.x * w0.y) +
                  texture(tex, vec2(texPos12.x, texPos0.y)).rgb * (w12.x * w0.y) +
                  texture(tex, vec2(texPos3.x, texPos0.y)).rgb * (w3.x * w0.y) +
                  texture(tex, vec2(texPos0.x, texPos12.y)).rgb * (w0.x * w12.y) +
                  texture(tex, texPos12).rgb * (w12.x * w12.y) +
                  texture(tex, vec2(texPos3.x, texPos12.y)).rgb * (w3.x * w12.y) +
                  texture(tex, vec2(texPos0.x, texPos3.y)).rgb * (w0.x * w3.y) +
                  texture(tex, vec2(texPos12.x, texPos3.y)).rgb * (w12.x * w3.y) +
                  texture(tex, vec2(texPos3.x, texPos3.y)).rgb * (w3.x * w3.y);
    
    return max(result, vec3(0.0));
}

void main() {
    vec2 texelSize = 1.0 / textureSize(currentFrame, 0);
    
    // Sample current frame
    vec3 currentColor = texture(currentFrame, TexCoords).rgb;
    
    // Get motion vector from G-buffer
    vec2 velocity = texture(gVelocity, TexCoords).xy;
    
    // Calculate previous frame UV coordinates
    vec2 prevUV = TexCoords - velocity;
    
    // Check if previous UV is within bounds
    if (prevUV.x < 0.0 || prevUV.x > 1.0 || prevUV.y < 0.0 || prevUV.y > 1.0) {
        // Out of bounds - use current frame only
        FragColor = vec4(currentColor, 1.0);
        return;
    }
    
    // Sample history frame with high-quality filtering
    vec3 historyColor = sampleCatmullRom(historyFrame, prevUV, textureSize(historyFrame, 0));
    
    // Convert to YCoCg for better temporal stability
    vec3 currentYCoCg = RGB2YCoCg(currentColor);
    vec3 historyYCoCg = RGB2YCoCg(historyColor);
    
    // Neighborhood sampling for variance estimation
    vec3 neighborSum = vec3(0.0);
    vec3 neighborSqSum = vec3(0.0);
    float sampleCount = 0.0;
    
    // 3x3 neighborhood sampling
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            vec3 neighbor = RGB2YCoCg(texture(currentFrame, TexCoords + offset).rgb);
            neighborSum += neighbor;
            neighborSqSum += neighbor * neighbor;
            sampleCount += 1.0;
        }
    }
    
    vec3 neighborMean = neighborSum / sampleCount;
    vec3 neighborVariance = (neighborSqSum / sampleCount) - (neighborMean * neighborMean);
    vec3 neighborStdDev = sqrt(max(neighborVariance, vec3(0.0)));
    
    // Clamp history to neighborhood (variance clipping) - looser bounds to reduce ghosting
    vec3 minColor = neighborMean - neighborStdDev * 1.2;  // Looser variance bounds
    vec3 maxColor = neighborMean + neighborStdDev * 1.2;
    vec3 clampedHistory = clamp(historyYCoCg, minColor, maxColor);
    
    // Calculate blend factor based on multiple criteria
    float velocityLength = length(velocity);
    float velocityFactor = clamp(velocityLength / VELOCITY_WEIGHT, 0.0, 1.0);
    
    // Luminance difference factor
    float lumaDiff = abs(currentYCoCg.x - clampedHistory.x);
    float lumaFactor = clamp(lumaDiff / LUMA_WEIGHT, 0.0, 1.0);
    
    // Distance from clamp (how much history was modified) - less aggressive
    float clampDistance = length(historyYCoCg - clampedHistory);
    float clampFactor = clamp(clampDistance * 2.0, 0.0, 1.0);
    
    // Combine factors for final blend weight
    float blendFactor = max(max(velocityFactor, lumaFactor), clampFactor);
    blendFactor = mix(MIN_BLEND_FACTOR, MAX_BLEND_FACTOR, blendFactor);
    
    // For the first few frames, use higher blend factor to establish history
    if (frameCounter < 15.0) {
        float warmupFactor = frameCounter / 15.0;
        blendFactor = mix(0.4, blendFactor, warmupFactor);  // Even less aggressive warmup
    }
    
    // Blend current and history
    vec3 finalYCoCg = mix(clampedHistory, currentYCoCg, blendFactor);
    
    // Convert back to RGB
    vec3 finalColor = YCoCg2RGB(finalYCoCg);
    
    FragColor = vec4(finalColor, 1.0);
} 