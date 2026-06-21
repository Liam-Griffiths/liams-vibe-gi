#version 430 core
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
uniform sampler2D gVelocity;     // Screen-space motion vector (UV delta), from G-buffer

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

// Quality-dependent controls
uniform int rayMarchSteps; // Number of steps along each ray (varies with quality)

// Sampling selector
uniform int samplingMethod; // 0..N per enum in C++

/**
 * Simplified Radiance Cascades for Stable Performance
 * Focus on dense, reliable sampling for smooth GI
 */

// Simple random function using spatial coordinates
float rand(vec2 co) {
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

// Interleaved gradient noise (Jimenez) - cheap, well-distributed per-pixel hash.
// Used to decorrelate sample patterns across neighbouring pixels and across frames
// so that under-sampling shows up as high-frequency noise (blurrable + temporally
// accumulatable) instead of stable low-frequency blotches.
float interleavedGradientNoise(vec2 p) {
    return fract(52.9829189 * fract(dot(p, vec2(0.06711056, 0.00583715))));
}

// Generate better distributed samples using spatial-temporal seeding
// Convert a unit-disk sample to hemisphere UV in [0,1]^2 suitable for getHemisphereSample
vec2 diskToHemisphereUV(vec2 d) {
    float angle = atan(d.y, d.x);               // [-pi, pi]
    float u = (angle / (2.0 * 3.14159));
    u = u < 0.0 ? u + 1.0 : u;                  // [0,1)
    float r = clamp(length(d), 0.0, 1.0);
    float v = r * r;                             // uniform in [0,1]
    return vec2(u, v);
}

vec2 getSample(int index, int totalSamples, vec2 screenPos) {
    // Multiple subdivision/sampling methods → first generate disk sample where applicable,
    // then convert to hemisphere UV expected by getHemisphereSample.
    float goldenAngle = 2.39996323; // golden spiral

    // Default: uniform hemisphere (ring stratified)
    vec2 disk = vec2(0.0);
    bool hasUV = false;
    vec2 uv = vec2(0.0);

    if (samplingMethod == 0) {
        // Cube Face Subdivision (approximate): stratified grid → map to disk via concentric mapping
        int grid = int(ceil(sqrt(float(totalSamples))));
        int x = index % grid;
        int y = index / grid;
        vec2 s = (vec2(x, y) + 0.5) / float(grid); // [0,1]
        // concentric map to disk (Peter Shirley)
        float a = 2.0 * s.x - 1.0;
        float b = 2.0 * s.y - 1.0;
        float r, phi;
        if (a == 0.0 && b == 0.0) {
            disk = vec2(0.0);
        } else if (abs(a) > abs(b)) {
            r = a;
            phi = (3.14159/4.0) * (b/a);
            disk = vec2(r * cos(phi), r * sin(phi));
        } else {
            r = b;
            phi = (3.14159/2.0) - (3.14159/4.0) * (a/b);
            disk = vec2(r * cos(phi), r * sin(phi));
        }
    } else if (samplingMethod == 1) {
        // Lat/Lon Subdivision → spiral on disk
        float t = (float(index) + 0.5) / float(totalSamples);
        float phi = 2.0 * 3.14159 * t;
        float r = sqrt(t);
        disk = vec2(r * cos(phi), r * sin(phi));
    } else if (samplingMethod == 2) {
        // Golden Spiral
        float r = sqrt(float(index) + 0.5) / sqrt(float(totalSamples));
        float angle = float(index) * goldenAngle;
        disk = vec2(r * cos(angle), r * sin(angle));
    } else if (samplingMethod == 3) {
        // Kogan Spiral (approx)
        float r = sqrt((float(index) + 0.5) / float(totalSamples));
        float angle = float(index) * (3.0 - sqrt(5.0));
        disk = vec2(r * cos(angle * 6.28318), r * sin(angle * 6.28318));
    } else if (samplingMethod == 4) {
        // Golden Hemisphere: generate uv directly
        float u = (float(index) + 0.5) / float(totalSamples);
        float v = fract(float(index) * 0.61803398875);
        uv = vec2(v, u); // angle=v, radial=u
        hasUV = true;
    } else if (samplingMethod == 5) {
        // Random Hemisphere → disk
        vec2 spatialSeed = screenPos * 73.0 + vec2(cascadeIndex * 37.0);
        float r = rand(spatialSeed + float(index));
        float a = rand(spatialSeed + float(index) * 1.37) * 6.28318;
        disk = vec2(sqrt(r) * cos(a), sqrt(r) * sin(a));
    } else if (samplingMethod == 6) {
        // Uniform Random Hemisphere: generate uv directly
        float u1 = fract(sin(float(index) * 12.9898 + dot(screenPos, vec2(78.233, 37.719))) * 43758.5453);
        float u2 = fract(sin(float(index) * 93.9898 + dot(screenPos, vec2(12.233, 17.719))) * 24634.6345);
        uv = vec2(u1, u2);
        hasUV = true;
    } else {
        // Uniform Hemisphere (ring stratified) → disk
        float t = (float(index) + 0.5) / float(totalSamples);
        float a = 6.28318 * t;
        float r = sqrt(t);
        disk = vec2(r * cos(a), r * sin(a));
    }

    return hasUV ? uv : diskToHemisphereUV(disk);
}

// Octahedral decode for normals
vec3 octDecode(vec2 e) {
    vec2 f = e * 2.0 - 1.0;
    vec3 n = vec3(f.x, f.y, 1.0 - abs(f.x) - abs(f.y));
    float t = clamp(-n.z, 0.0, 1.0);
    n.x += (n.x >= 0.0 ? -t : t);
    n.y += (n.y >= 0.0 ? -t : t);
    return normalize(n);
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
    // Decode octahedral normal
    vec2 enc = texture(gNormal, uv).rg;
    vec3 normal = octDecode(enc);
    if (length(normal) < 0.1) return vec4(0.0, 0.0, 0.0, 1.0);
    
    // Convert to world space
    vec3 worldPos = (invView * vec4(viewPos, 1.0)).xyz;
    mat3 invViewNormal = mat3(invView);
    vec3 worldNormal = invViewNormal * normal;
    
    vec3 gi = vec3(0.0);
    int numHits = 0;
    
    // Angular sample budget comes from the per-cascade allocation (near cascades few,
    // far cascades many - see RadianceCascades::initializeBandLimitingParameters). The low
    // floor lets cascade 0 be genuinely cheap; temporal accumulation recovers the variance.
    int numSamples = clamp(angularSamples, 4, 128);
    
    float minDist = minDistance;
    float maxDist = maxDistance;
    float thickness = 0.08 + minDist * 0.02; // Adaptive thickness
    
    int numSteps = max(1, rayMarchSteps); // Steps along each ray (set from CPU)

    // Per-pixel + per-frame Cranley-Patterson rotation. Decorrelates the (otherwise
    // identical-per-pixel, identical-per-frame) stratified pattern so the residual
    // error is high-frequency and averages out under temporal accumulation. This is
    // the primary fix for the blotchy GI look.
    float frameSeed = float(frameCounter & 255);
    vec2 cpRotation = vec2(
        interleavedGradientNoise(gl_FragCoord.xy + frameSeed * vec2(11.0, 23.0)),
        interleavedGradientNoise(gl_FragCoord.xy + frameSeed * vec2(37.0, 17.0) + 113.0)
    );

    for (int s = 0; s < numSamples; ++s) {
        // Get well-distributed sample direction, then toroidally shift it (CP rotation).
        vec2 sampleUV = fract(getSample(s, numSamples, uv) + cpRotation);
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
                // Decode sample normal
                vec2 sampleEnc = texture(gNormal, sampleUV).rg;
                vec3 sampleViewNormal = octDecode(sampleEnc);
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
        
        // Cascade merge operator M(R_near, R_far) = I_near + exp(-tau_near) * I_far
        // (arXiv:2408.14425 eq. 11). This ray found no occluder within the cascade's
        // interval, so its near-field transmittance is ~1 and the far (previous) cascade's
        // radiance propagates through essentially unattenuated - no lossy fixed weight.
        if (!hit && hasPreviousCascade) {
            vec3 prevRadiance = texture(previousCascade, TexCoords).rgb;
            float cosTerm = max(0.0, dot(worldNormal, worldDir));
            float transmittance = 1.0; // near interval was clear for this direction

            // Only the overlap fade remains, purely to avoid double-counting energy in the
            // band where this cascade's interval overlaps the next one's.
            if (enableCascadeBlending) {
                float blendFactor = smoothstep(cascadeOverlapStart, cascadeOverlapEnd, t);
                transmittance *= (1.0 - blendFactor);
            }

            gi += prevRadiance * transmittance * cosTerm;
        }
    }
    
    if (numSamples > 0) {
        gi /= float(numSamples);
    }
    float beta = float(numSamples - numHits) / float(numSamples);
    
    // Temporal accumulation with motion-vector reprojection.
    // Sampling history at the *reprojected* UV (TexCoords - velocity) instead of the
    // current pixel is what lets the per-frame-rotated samples (above) accumulate into
    // smooth GI under camera motion instead of smearing/ghosting.
    if (useTemporalAccumulation && frameCounter > 0) {
        vec2 velocity = texture(gVelocity, TexCoords).xy;
        vec2 prevUV = TexCoords - velocity;

        // Disocclusion / off-screen guard: reject history that reprojects outside the
        // frame so newly-revealed surfaces don't inherit stale lighting.
        bool validHistory = all(greaterThanEqual(prevUV, vec2(0.0))) &&
                            all(lessThanEqual(prevUV, vec2(1.0)));

        if (validHistory) {
            vec4 temporal = texture(temporalBuffer, prevUV);
            vec3 temporalGi = temporal.rgb;
            float temporalBeta = temporal.a;

            if (length(temporalGi) > 0.001) {
                // Base responsiveness (cascade 0 reacts faster). Bias toward the current
                // frame as on-screen motion grows: this is a cheap stand-in for a full
                // neighbourhood variance clamp and suppresses ghosting trails while the
                // estimate is still settling.
                float baseBlend = (cascadeIndex == 0) ? 0.35 : 0.25;
                float motion = clamp(length(velocity) * 64.0, 0.0, 1.0);
                float blendFactor = mix(baseBlend, 1.0, motion);
                gi = mix(temporalGi, gi, blendFactor);
                beta = mix(temporalBeta, beta, blendFactor);
            }
        }
    }
    
    return vec4(gi, beta);
}

void main() {
    vec4 radiance = computeRadiance(TexCoords, cascadeIndex);
    FragColor = radiance;
} 