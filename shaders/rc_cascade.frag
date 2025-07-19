#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D gLinearDepth;
uniform sampler2D gEmission; // Add emission texture for emissive surfaces
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
    
    // Higher sampling for smooth emission lighting
    int baseSamples = 20; // More samples for smoother emission
    int extraSamples = max(0, (activeCascades - 2) * 4); // +4 samples per cascade above 2
    int numSamples = baseSamples + extraSamples - index * 2; // Gentler reduction for higher cascades
    numSamples = max(12, numSamples); // Higher minimum for smooth emission
    
    // Ultra mode gets more samples for cascade 0-2 for better emission quality
    if (activeCascades >= 6 && index < 3) {
        numSamples += 8; // More samples for emission quality in detailed cascades
    }
    
    // Cascade 0 gets extra samples for smooth emission
    if (index == 0) {
        numSamples += 6; // More samples for smooth emission in primary cascade
    }
    
    float minDist = pow(2.0, float(index)) + 0.01;
    float maxDist = pow(2.0, float(index + 1));
    float thickness = 0.06; // Reduced for higher precision
    
    // Balanced raymarching for good quality and performance
    int numSteps = activeCascades >= 6 ? 10 : 8; // Reasonable step count
    
    // High-quality sampling distribution for smooth emission
    vec2 spatialSeed = worldPos.xz * 7.0 + vec2(index * 37.0, index * 73.0); // Improved world-based seed
    
    for (int s = 0; s < numSamples; ++s) {
        // Low-discrepancy sampling pattern for more even distribution and smoother emission
        float phi = float(s) * 2.399963; // Golden angle for better distribution
        float r = sqrt(float(s) + 0.5) / sqrt(float(numSamples)); // Even radial distribution
        
        vec2 rnd = vec2(
            rand(spatialSeed + vec2(s * 1.618, phi)), // Use golden ratio for better spacing
            rand(spatialSeed + vec2(r * 2.718, s * 3.141)) // Use mathematical constants for good distribution
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
                
                // Large-area optimized emission sampling for ultra-smooth lighting
                vec3 emissionContribution = vec3(0.0);
                float totalEmissionWeight = 0.0;
                
                // Poisson disk sampling pattern for optimal large-area coverage
                vec2 poissonSamples[16] = vec2[](
                    vec2(-0.94201624, -0.39906216), vec2(0.94558609, -0.76890725),
                    vec2(-0.094184101, -0.92938870), vec2(0.34495938, 0.29387760),
                    vec2(-0.91588581, 0.45771432), vec2(-0.81544232, -0.87912464),
                    vec2(-0.38277543, 0.27676845), vec2(0.97484398, 0.75648379),
                    vec2(0.44323325, -0.97511554), vec2(0.53742981, -0.47373420),
                    vec2(-0.26496911, -0.41893023), vec2(0.79197514, 0.19090188),
                    vec2(-0.24188840, 0.99706507), vec2(-0.81409955, 0.91437590),
                    vec2(0.19984126, 0.78641367), vec2(0.14383161, -0.14100790)
                );
                
                vec2 texelSize = 1.0 / textureSize(gEmission, 0);
                float samplingRadius = 6.0; // Much larger radius for smoother emission
                
                // Hierarchical sampling: center + wide pattern
                // Center sample (highest weight)
                vec3 centerEmission = texture(gEmission, sampleUV).rgb;
                emissionContribution += centerEmission * 2.0; // Center gets double weight
                totalEmissionWeight += 2.0;
                
                // Adaptive sampling: check if emission is uniform for optimization
                vec3 cornerEmission = texture(gEmission, sampleUV + texelSize * samplingRadius * vec2(0.707, 0.707)).rgb;
                vec3 emissionVariance = abs(centerEmission - cornerEmission);
                float emissionUniformity = (emissionVariance.r + emissionVariance.g + emissionVariance.b) / 3.0;
                
                // If emission is very uniform, use fewer samples for optimization
                int adaptiveSamples = (emissionUniformity < 0.1) ? 8 : 16; // Half samples for uniform areas
                
                // Wide Poisson disk pattern for large-area smoothing
                for (int i = 0; i < adaptiveSamples; ++i) {
                    vec2 offset = poissonSamples[i] * texelSize * samplingRadius;
                    vec2 emissionUV = sampleUV + offset;
                    
                    if (emissionUV.x >= 0.0 && emissionUV.x <= 1.0 && emissionUV.y >= 0.0 && emissionUV.y <= 1.0) {
                        vec3 sampleEmission = texture(gEmission, emissionUV).rgb;
                        
                        // Distance-based weighting for smooth falloff
                        float sampleDistance = length(offset) / (texelSize.x * samplingRadius);
                        float weight = exp(-sampleDistance * 0.5); // Gaussian-like falloff
                        
                        emissionContribution += sampleEmission * weight;
                        totalEmissionWeight += weight;
                    }
                }
                
                if (totalEmissionWeight > 0.0) {
                    emissionContribution /= totalEmissionWeight; // Weighted average of large area
                    
                    // Distance-based falloff for energy conservation
                    float emissionDistance = length(worldSamplePos - worldPos);
                    float emissionFalloff = 1.0 / (1.0 + emissionDistance * 0.012); // Even gentler for smooth gradients
                    emissionContribution *= 10.0 * emissionFalloff; // Slightly higher for larger area sampling
                }
                
                // Ultra mode: Add multi-bounce approximation
                vec3 finalRadiance = direct + emissionContribution;
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
            // Emission-aware temporal blending for smoother emissive lighting
            vec3 currentEmission = texture(gEmission, TexCoords).rgb;
            float emissionLuminance = dot(currentEmission, vec3(0.299, 0.587, 0.114));
            
            // Base blending - enough current frame to prevent blotchiness
            float blendFactor = 0.6; // 60% current frame, 40% temporal for balance
            
                         // Near emissive surfaces, use much more temporal accumulation for ultra-smooth results
             if (emissionLuminance > 0.1) {
                 blendFactor = 0.25; // 25% current frame, 75% temporal near emission for maximum smoothness
             } else if (emissionLuminance > 0.01) {
                 blendFactor = 0.35; // 35% current frame, 65% temporal for indirect emission influence
             }
            
                         if (activeCascades >= 6) {
                 // Ultra mode: ultra-smooth near emission, responsive elsewhere
                 if (emissionLuminance > 0.1) {
                     blendFactor = 0.2; // 20% current, 80% temporal for maximum smoothness
                 } else if (emissionLuminance > 0.01) {
                     blendFactor = 0.3; // 30% current, 70% temporal for indirect emission
                 } else {
                     blendFactor = 0.7; // 70% current, 30% temporal away from emission
                 }
                 
                 // Variance-based adaptive blending for Ultra mode (only away from emission)
                 vec3 giVariance = abs(gi - temporalGi);
                 float varianceAmount = (giVariance.r + giVariance.g + giVariance.b) / 3.0;
                 if (varianceAmount > 0.05 && emissionLuminance < 0.01) { // Only be responsive away from emission
                     blendFactor = 0.85; // 85% current frame when there's change but not near emission
                 }
             } else {
                 // Standard mode: emission-aware blending
                 if (emissionLuminance > 0.1) {
                     blendFactor = 0.3; // 30% current, 70% temporal near emission
                 } else if (emissionLuminance > 0.01) {
                     blendFactor = 0.4; // 40% current, 60% temporal for indirect emission  
                 } else {
                     blendFactor = 0.65; // 65% current, 35% temporal away from emission
                 }
             }
            
            // For first few frames, use more current frame for quick convergence
            if (frameCounter < 15) {
                float convergence = float(frameCounter) / 15.0;
                blendFactor = mix(0.9, blendFactor, convergence); // Quick convergence
            }
            
            // Exponential moving average - blend from temporal to current (parameters fixed!)
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