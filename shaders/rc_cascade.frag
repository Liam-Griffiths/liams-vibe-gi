#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

// G-buffer inputs
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D gLinearDepth;
uniform sampler2D gEmission;
uniform sampler2D previousCascade;
uniform sampler2D temporalBuffer;

// Cascade parameters
uniform int cascadeIndex;
uniform float minDistance;
uniform float maxDistance;
uniform int angularSamples;
uniform bool hasPreviousCascade;

// New cascade blending parameters for smooth transitions
uniform float cascadeOverlapStart;
uniform float cascadeOverlapEnd;
uniform bool enableCascadeBlending;

// Rendering parameters
uniform int frameCounter;
uniform bool useTemporalAccumulation;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 invView;
uniform float time;

// Lighting parameters
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform float lightRadius;

/**
 * Simplified Radiance Cascades for Stable Performance
 * Focus on dense, reliable sampling for smooth GI
 */

// Simple random function using spatial coordinates
float rand(vec2 co) {
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

// Generate better distributed samples using spatial-temporal seeding
vec2 getSample(int index, int totalSamples, vec2 screenPos) {
    // Use golden ratio for better distribution
    float goldenAngle = 2.39996323;
    float radius = sqrt(float(index) + 0.5) / sqrt(float(totalSamples));
    float angle = float(index) * goldenAngle;
    
    // Add small spatial variation to break up patterns
    vec2 spatialSeed = screenPos * 73.0 + vec2(cascadeIndex * 37.0);
    angle += rand(spatialSeed) * 0.5; // Small random offset
    
    return vec2(radius * cos(angle), radius * sin(angle));
}

// Get hemisphere sample direction with better distribution
vec3 getHemisphereSample(vec3 normal, vec2 uv) {
    // Convert to hemisphere coordinates
    float phi = 2.0 * 3.14159 * uv.x;
    float cosTheta = sqrt(1.0 - uv.y); // Cosine weighted for better diffuse sampling
    float sinTheta = sqrt(uv.y);
    
    vec3 arbitrary = abs(normal.z) < 0.9 ? vec3(0,0,1) : vec3(1,0,0);
    vec3 tangent = normalize(cross(normal, arbitrary));
    vec3 bitangent = cross(normal, tangent);
    return cos(phi) * sinTheta * tangent + sin(phi) * sinTheta * bitangent + cosTheta * normal;
}

// Soft area light attenuation
float calculateSoftAttenuation(float distance, float radius) {
    float normalizedDist = distance / radius;
    float falloff = 1.0 / (1.0 + normalizedDist * normalizedDist * 0.25);
    float maxRange = radius * 3.0;
    float rangeFactor = 1.0 - smoothstep(maxRange * 0.7, maxRange, distance);
    return falloff * rangeFactor;
}

vec4 computeRadiance(vec2 uv, int index) {
    vec3 viewPos = texture(gPosition, uv).xyz;
    // Reconstruct normal from RG16F format
    vec2 normalXY = texture(gNormal, uv).rg;
    float normalZ = sqrt(max(0.0, 1.0 - dot(normalXY, normalXY)));
    vec3 normal = normalize(vec3(normalXY, normalZ));
    if (length(normal) < 0.1) return vec4(0.0, 0.0, 0.0, 1.0);
    
    // Convert to world space
    vec3 worldPos = (invView * vec4(viewPos, 1.0)).xyz;
    mat3 invViewNormal = mat3(invView);
    vec3 worldNormal = invViewNormal * normal;
    
    vec3 gi = vec3(0.0);
    int numHits = 0;
    
    // Enhanced sampling parameters for ultra-smooth results
    int numSamples = max(16, angularSamples);
    numSamples = min(numSamples, 80); // Increased cap for cascade 0 (was 48)
    
    float minDist = minDistance;
    float maxDist = maxDistance;
    float thickness = 0.08 + minDist * 0.02; // Adaptive thickness
    
    int numSteps = 8; // More steps for better hit detection
    
    for (int s = 0; s < numSamples; ++s) {
        // Get well-distributed sample direction
        vec2 sampleUV = getSample(s, numSamples, uv);
        vec3 worldDir = getHemisphereSample(worldNormal, sampleUV);
        
        float stepSize = (maxDist - minDist) / float(numSteps);
        float t = minDist;
        bool hit = false;
        
        while (t < maxDist) {
            vec3 worldSamplePos = worldPos + worldDir * t;
            // Project back to view space for sampling
            vec4 viewSample = view * vec4(worldSamplePos, 1.0);
            vec4 clip = projection * viewSample;
            if (clip.w <= 0.0) break;
            vec2 sampleUV = (clip.xy / clip.w) * 0.5 + 0.5;
            if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0) break;
            
            float sampledDepth = texture(gLinearDepth, sampleUV).r;
            float projectedDepth = -viewSample.z;
            if (sampledDepth > 0.0 && abs(projectedDepth - sampledDepth) < thickness) {
                vec3 sampleAlbedo = texture(gAlbedo, sampleUV).rgb;
                // Reconstruct sample normal from RG16F format
                vec2 sampleNormalXY = texture(gNormal, sampleUV).rg;
                float sampleNormalZ = sqrt(max(0.0, 1.0 - dot(sampleNormalXY, sampleNormalXY)));
                vec3 sampleViewNormal = normalize(vec3(sampleNormalXY, sampleNormalZ));
                vec3 sampleWorldNormal = invViewNormal * sampleViewNormal;
                vec3 sampleToLight = lightPos - worldSamplePos;
                float distToLight = length(sampleToLight);
                vec3 lightDir = sampleToLight / distToLight;
                float diff = max(dot(sampleWorldNormal, lightDir), 0.0);
                
                float att = calculateSoftAttenuation(distToLight, lightRadius);
                vec3 direct = sampleAlbedo * lightColor * diff * att;
                float cosTerm = max(0.0, dot(worldNormal, worldDir));
                
                // Add emission contribution (increased for better range)
                vec3 emission = texture(gEmission, sampleUV).rgb;
                
                gi += (direct + emission * 8.0) * cosTerm; // Increased emission multiplier for better range
                hit = true;
                numHits++;
                break;
            }
            t += stepSize;
        }
        
        // Enhanced cascade blending with previous cascade
        if (!hit && hasPreviousCascade) {
            vec4 prevData = texture(previousCascade, TexCoords);
            vec3 prevRadiance = prevData.rgb;
            float prevBeta = prevData.a;
            
            // Implement cascade overlap and soft blending
            float blendWeight = 0.4 * (1.0 - prevBeta);
            
            // Apply distance-based blend factor if cascade blending is enabled
            if (enableCascadeBlending) {
                float distanceToSample = t;
                float blendFactor = smoothstep(cascadeOverlapStart, cascadeOverlapEnd, distanceToSample);
                blendWeight *= (1.0 - blendFactor); // Fade out as we approach next cascade range
            }
            
            gi += prevRadiance * max(0.0, dot(worldNormal, worldDir)) * blendWeight;
        }
    }
    
    if (numSamples > 0) {
        gi /= float(numSamples);
    }
    float beta = float(numSamples - numHits) / float(numSamples);
    
    // Enhanced temporal accumulation with more responsive blending
    if (useTemporalAccumulation && frameCounter > 0) {
        vec4 temporal = texture(temporalBuffer, TexCoords);
        vec3 temporalGi = temporal.rgb;
        float temporalBeta = temporal.a;
        
        if (length(temporalGi) > 0.001) {
            // Adaptive blending based on cascade index - cascade 0 more responsive
            float blendFactor = (cascadeIndex == 0) ? 0.35 : 0.25; // 35% for cascade 0, 25% for others
            gi = mix(temporalGi, gi, blendFactor);
            beta = mix(temporalBeta, beta, blendFactor);
        }
    }
    
    return vec4(gi, beta);
}

void main() {
    vec4 radiance = computeRadiance(TexCoords, cascadeIndex);
    FragColor = radiance;
} 