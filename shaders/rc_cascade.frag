#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D gLinearDepth;
uniform sampler2D previousCascade;
uniform sampler2D temporalBuffer;
uniform int cascadeIndex;
uniform int frameCounter;
uniform bool useTemporalAccumulation;
uniform mat4 view;
uniform mat4 projection;
uniform float time;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform float lightRadius; // New: light size parameter
uniform mat4 invView; // New: inverse view for world space
uniform int activeCascades; // New: for quality-aware computation

// Much more stable random function with spatial seeds only
float rand(vec2 co) {
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 getHemisphereSample(vec3 normal, vec2 uv) {
    float phi = 2.0 * 3.14159 * uv.x;
    float cosTheta = sqrt(1.0 - uv.y);
    float sinTheta = sqrt(uv.y);
    vec3 arbitrary = abs(normal.z) < 0.9 ? vec3(0,0,1) : vec3(1,0,0);
    vec3 tangent = normalize(cross(normal, arbitrary));
    vec3 bitangent = cross(normal, tangent);
    return cos(phi) * sinTheta * tangent + sin(phi) * sinTheta * bitangent + cosTheta * normal;
}

// Soft area light attenuation for GI (matching final composite)
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
    
    // Quality-aware sampling based on total cascades (higher quality = more cascades = more samples)
    int baseSamples = 8; // Base sample count
    int extraSamples = max(0, (activeCascades - 2) * 3); // +3 samples per cascade above 2
    int numSamples = baseSamples + extraSamples - index * 2; // Still reduce for higher cascade indices
    numSamples = max(4, numSamples); // Minimum 4 samples
    
    // Ultra mode gets even more samples for cascade 0-2
    if (activeCascades >= 6 && index < 3) {
        numSamples += 6; // Ultra boost for fine detail cascades
    }
    
    float minDist = pow(2.0, float(index)) + 0.01;
    float maxDist = pow(2.0, float(index + 1));
    float thickness = 0.08; // Slightly increased for more reliable hits
    
    // Ultra mode: higher precision raymarching
    int numSteps = activeCascades >= 6 ? 8 : 6;
    
    // More stable world-space sampling with better distribution
    vec2 spatialSeed = worldPos.xz * 10.0 + vec2(index * 31.0, index * 67.0); // World-based seed
    
    for (int s = 0; s < numSamples; ++s) {
        // Better distributed sampling pattern for smoother results
        vec2 rnd = vec2(
            rand(spatialSeed + vec2(s * 1.3, s * 2.7)),
            rand(spatialSeed + vec2(s * 3.7, s * 4.1))
        );
        
        vec3 worldDir = getHemisphereSample(worldNormal, rnd); // World space direction
        float stepSize = (maxDist - minDist) / float(numSteps); // Quality-aware step count
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
            if (sampledDepth > 0.0 && abs(projectedDepth - sampledDepth) < thickness * projectedDepth) {
                vec3 sampleAlbedo = texture(gAlbedo, sampleUV).rgb;
                // Reconstruct sample normal from RG16F format
        vec2 sampleNormalXY = texture(gNormal, sampleUV).rg;
        float sampleNormalZ = sqrt(max(0.0, 1.0 - dot(sampleNormalXY, sampleNormalXY)));
        vec3 sampleViewNormal = normalize(vec3(sampleNormalXY, sampleNormalZ));
                vec3 sampleWorldNormal = invViewNormal * sampleViewNormal;
                vec3 sampleToLight = lightPos - worldSamplePos; // lightPos now world space
                float distToLight = length(sampleToLight);
                vec3 lightDir = sampleToLight / distToLight;
                float diff = max(dot(sampleWorldNormal, lightDir), 0.0);
                
                // Use soft attenuation consistent with main lighting
                float att = calculateSoftAttenuation(distToLight, lightRadius);
                
                vec3 direct = sampleAlbedo * lightColor * diff * att;
                float cosTerm = max(0.0, dot(worldNormal, worldDir));
                
                // Ultra mode: Add multi-bounce approximation
                vec3 finalRadiance = direct;
                if (activeCascades >= 6 && index < 2) {
                    // Approximate second bounce using albedo and average scene illumination
                    vec3 multiBounce = sampleAlbedo * lightColor * 0.02 * att; // Barely noticeable second bounce
                    finalRadiance += multiBounce;
                }
                
                gi += finalRadiance * cosTerm * 1.0; // Reduced multiplier for better balance
                hit = true;
                numHits++;
                break;
            }
            t += stepSize;
        }
        
        // Improved fallback with smoother blending
        if (!hit && index < 7) {
            vec3 prev = texture(previousCascade, TexCoords).rgb;
            float fallbackStrength = 0.4; // Increased for smoother fallback
            gi += prev * max(0.0, dot(worldNormal, worldDir)) * fallbackStrength;
        }
    }
    
    gi /= float(numSamples);
    float beta = float(numSamples - numHits) / float(numSamples);
    
    // Quality-aware temporal accumulation for stability
    if (useTemporalAccumulation) {
        vec4 temporal = texture(temporalBuffer, TexCoords);
        vec3 temporalGi = temporal.rgb;
        float temporalBeta = temporal.a;
        
        if (length(temporalGi) > 0.001) {
            // Ultra mode: More sophisticated temporal filtering
            float blendFactor = 0.15; // Base blend factor
            
            if (activeCascades >= 6) {
                // Ultra mode: More stable temporal accumulation
                blendFactor = 0.10; // Slower accumulation for higher stability
                
                // Variance-based adaptive blending for Ultra mode
                vec3 giVariance = abs(gi - temporalGi);
                float varianceAmount = (giVariance.r + giVariance.g + giVariance.b) / 3.0;
                if (varianceAmount > 0.1) {
                    blendFactor = mix(0.10, 0.25, min(varianceAmount / 0.5, 1.0)); // Adapt to changes
                }
            } else {
                // Standard mode: regular blending
                blendFactor = 0.20;
            }
            
            // For first few frames, use even gentler blending
            if (frameCounter < 60) {
                float warmupFactor = float(frameCounter) / 60.0;
                blendFactor = mix(0.05, blendFactor, warmupFactor); // Gradual warmup
            }
            
            // Exponential moving average instead of simple mix to prevent accumulation
            gi = mix(gi, temporalGi, blendFactor);
            beta = mix(beta, temporalBeta, blendFactor);
            
            // Energy conservation - clamp to prevent infinite accumulation
            float maxEnergy = activeCascades >= 6 ? 3.0 : 2.0; // Ultra mode allows more energy
            gi = clamp(gi, vec3(0.0), vec3(maxEnergy));
        }
    }
    
    return vec4(gi, beta);
}

void main() {
    vec4 radiance = computeRadiance(TexCoords, cascadeIndex);
    FragColor = radiance;
} 