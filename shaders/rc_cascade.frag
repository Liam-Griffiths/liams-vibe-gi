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
    vec3 normal = normalize(texture(gNormal, uv).xyz);
    if (length(normal) < 0.1) return vec4(0.0, 0.0, 0.0, 1.0);
    
    // Convert to world space
    vec3 worldPos = (invView * vec4(viewPos, 1.0)).xyz;
    mat3 invViewNormal = mat3(invView);
    vec3 worldNormal = invViewNormal * normal;
    
    vec3 gi = vec3(0.0);
    int numHits = 0;
    // LOD-based sampling: fewer samples for higher cascades (they contribute less)
    int numSamples = max(4, 10 - index * 2); // 10, 8, 6, 4, 4, 4 samples per cascade
    float minDist = pow(2.0, float(index)) + 0.01;
    float maxDist = pow(2.0, float(index + 1));
    float thickness = 0.08; // Slightly increased for more reliable hits
    
    // More stable world-space sampling with better distribution
    vec2 spatialSeed = worldPos.xz * 10.0 + vec2(index * 31.0, index * 67.0); // World-based seed
    
    for (int s = 0; s < numSamples; ++s) {
        // Better distributed sampling pattern for smoother results
        vec2 rnd = vec2(
            rand(spatialSeed + vec2(s * 1.3, s * 2.7)),
            rand(spatialSeed + vec2(s * 3.7, s * 4.1))
        );
        
        vec3 worldDir = getHemisphereSample(worldNormal, rnd); // World space direction
        float stepSize = (maxDist - minDist) / 6.0; // More steps for better quality
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
                vec3 sampleViewNormal = normalize(texture(gNormal, sampleUV).xyz);
                vec3 sampleWorldNormal = invViewNormal * sampleViewNormal;
                vec3 sampleToLight = lightPos - worldSamplePos; // lightPos now world space
                float distToLight = length(sampleToLight);
                vec3 lightDir = sampleToLight / distToLight;
                float diff = max(dot(sampleWorldNormal, lightDir), 0.0);
                
                // Use soft attenuation consistent with main lighting
                float att = calculateSoftAttenuation(distToLight, lightRadius);
                
                vec3 direct = sampleAlbedo * lightColor * diff * att;
                float cosTerm = max(0.0, dot(worldNormal, worldDir));
                gi += direct * cosTerm * 1.2; // Slightly reduced multiplier for balance
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
    
    // STRONG temporal accumulation for rock-solid stability
    if (useTemporalAccumulation) {
        vec4 temporal = texture(temporalBuffer, TexCoords);
        vec3 temporalGi = temporal.rgb;
        float temporalBeta = temporal.a;
        
        if (length(temporalGi) > 0.001) {
            // VERY strong temporal blending for ultra-smooth GI
            float blendFactor = 0.95; // Increased from 0.85 for maximum smoothness
            
            // For first few frames, use faster convergence to smooth result
            if (frameCounter < 30) {
                blendFactor = 0.7 + (float(frameCounter) / 30.0) * 0.25; // 0.7 -> 0.95 over 30 frames
            }
            
            // Exponential moving average instead of simple mix to prevent accumulation
            gi = mix(gi, temporalGi, blendFactor);
            beta = mix(beta, temporalBeta, blendFactor);
            
            // Energy conservation - clamp to prevent infinite accumulation
            gi = clamp(gi, vec3(0.0), vec3(2.0)); // Reasonable upper bound
        }
    }
    
    return vec4(gi, beta);
}

void main() {
    vec4 radiance = computeRadiance(TexCoords, cascadeIndex);
    FragColor = radiance;
} 