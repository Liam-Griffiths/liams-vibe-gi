#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D shadowMap;
uniform sampler2D rcTexture[6]; // Support up to 6 cascades
uniform sampler2D ssaoTexture; // New: SSAO texture
uniform int activeCascades; // Number of active cascades for current quality level

// Lighting uniforms
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform mat4 lightSpaceMatrix;
uniform mat4 view;
uniform float lightRadius; // New: light size parameter

// SSGI parameters
uniform float ssgiStrength;
uniform float ambientStrength;
uniform float ssaoStrength; // New: SSAO strength

// Shadow calculation (same as lighting.frag)
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    float currentDepth = projCoords.z;
    
    float bias = max(0.0001 * (1.0 - dot(normal, lightDir)), 0.00001);
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -2; x <= 2; ++x)
    {
        for(int y = -2; y <= 2; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 25.0;
    
    if(projCoords.z > 1.0)
        shadow = 0.0;
        
    return shadow;
}

// Soft area light attenuation - much more artistic and realistic
float calculateSoftAttenuation(float distance, float radius) {
    // Smooth falloff that starts gentle and becomes more pronounced
    float normalizedDist = distance / radius;
    
    // Windowing function that's very soft near the light
    float falloff = 1.0 / (1.0 + normalizedDist * normalizedDist * 0.25);
    
    // Add smooth cutoff at reasonable distance (3x radius)
    float maxRange = radius * 3.0;
    float rangeFactor = 1.0 - smoothstep(maxRange * 0.7, maxRange, distance);
    
    return falloff * rangeFactor;
}

void main()
{
    vec3 position = texture(gPosition, TexCoords).xyz;
    // Reconstruct normal from RG16F format
    vec2 normalXY = texture(gNormal, TexCoords).rg;
    float normalZ = sqrt(max(0.0, 1.0 - dot(normalXY, normalXY)));
    vec3 normal = normalize(vec3(normalXY, normalZ));
    vec3 albedo = texture(gAlbedo, TexCoords).rgb;
    
    // Early exit for background pixels
    if (length(normal) < 0.1) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    
    vec3 fragPos = position; // Already in view space
    vec3 worldNormal = normalize(normal); // Already in view space
    
    // Transform to world space for shadow calculation
    vec4 fragPosLightSpace = lightSpaceMatrix * inverse(view) * vec4(fragPos, 1.0);
    
    // Light direction in view space
    vec3 lightDir = normalize((view * vec4(lightPos, 1.0)).xyz - fragPos);
    float lightDistance = length((view * vec4(lightPos, 1.0)).xyz - fragPos);
    
    // Use new soft attenuation based on light radius
    float attenuation = calculateSoftAttenuation(lightDistance, lightRadius);
    
    float shadow = ShadowCalculation(fragPosLightSpace, worldNormal, lightDir);
    
    // Calculate raw lighting components (WITHOUT albedo yet)
    float nDotL = max(dot(worldNormal, lightDir), 0.0);
    vec3 lightRadiance = lightColor * attenuation;
    
    // Direct diffuse lighting (will be modulated by albedo)
    vec3 directDiffuse = nDotL * lightRadiance * (1.0 - shadow);
    
    // Specular lighting (NOT modulated by albedo - it's a surface reflection)
    vec3 viewDir = normalize(-fragPos);
    vec3 reflectDir = reflect(-lightDir, worldNormal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64);
    vec3 directSpecular = 0.2 * spec * lightRadiance * (1.0 - shadow);
    
    // Calculate indirect lighting from radiance cascades with smooth interpolation
    vec3 indirectDiffuse = vec3(0.0);
    float totalWeight = 0.0;
    
    // First, collect all cascade contributions with quality-aware smooth sampling
    vec3 cascadeContributions[6];
    float cascadeWeights[6];
    
    for (int i = 0; i < activeCascades; ++i) {
        // Quality-aware sampling with better upsampling for Ultra mode
        vec3 smoothGi = vec3(0.0);
        float smoothBeta = 0.0;
        
        // Ultra mode: Enhanced sampling for all cascades
        if (activeCascades >= 6 && i >= 1) {
            vec2 texelSize = 1.0 / textureSize(rcTexture[i], 0);
            
            // Ultra mode: 5x5 high-quality upsampling kernel for smoother results
            float totalWeight = 0.0;
            for (int x = -2; x <= 2; ++x) {
                for (int y = -2; y <= 2; ++y) {
                    vec2 sampleCoord = TexCoords + vec2(x, y) * texelSize * 0.4;
                    vec4 sampleData = texture(rcTexture[i], sampleCoord);
                    
                    // Gaussian-like weights for ultra-smooth upsampling
                    float dist = sqrt(float(x*x + y*y));
                    float weight = exp(-dist * dist * 0.5); // Gaussian falloff
                    smoothGi += sampleData.rgb * weight;
                    smoothBeta += sampleData.a * weight;
                    totalWeight += weight;
                }
            }
            smoothGi /= totalWeight;
            smoothBeta /= totalWeight;
        }
        // Standard mode: 3x3 upsampling for cascades 2+
        else if (i >= 2) {
            vec2 texelSize = 1.0 / textureSize(rcTexture[i], 0);
            
            // 3x3 smooth upsampling kernel
            for (int x = -1; x <= 1; ++x) {
                for (int y = -1; y <= 1; ++y) {
                    vec2 sampleCoord = TexCoords + vec2(x, y) * texelSize * 0.5;
                    vec4 sampleData = texture(rcTexture[i], sampleCoord);
                    
                    // Gaussian-like weights for smooth upsampling
                    float weight = 1.0 / (1.0 + abs(float(x)) + abs(float(y)));
                    smoothGi += sampleData.rgb * weight;
                    smoothBeta += sampleData.a * weight;
                }
            }
            smoothGi /= 9.0; // Normalize by sample count
            smoothBeta /= 9.0;
        } else {
            // High resolution cascades can be sampled directly
            vec4 cascadeData = texture(rcTexture[i], TexCoords);
            smoothGi = cascadeData.rgb;
            smoothBeta = cascadeData.a;
        }
        
        cascadeContributions[i] = smoothGi;
        cascadeWeights[i] = smoothBeta;
    }
    
    // Now blend cascades smoothly with quality-aware weighting
    for (int i = 0; i < activeCascades; ++i) {
        vec3 cascadeGi = cascadeContributions[i];
        float cascadeBeta = cascadeWeights[i];
        
        // Quality-aware cascade weighting
        float spatialWeight;
        if (activeCascades >= 6) {
            // Ultra mode: More sophisticated cascade weighting
            spatialWeight = pow(0.75, float(i)); // Gentler falloff for more cascades
        } else {
            // Standard mode: normal falloff
            spatialWeight = pow(0.8, float(i));
        }
        
        float betaWeight = cascadeBeta;
        
        // Ultra mode: Enhanced inter-cascade smoothing
        if (activeCascades >= 6 && i > 0 && i < (activeCascades - 1)) {
            // Ultra mode: smoother blending with neighboring cascades
            vec3 prevCascade = cascadeContributions[i-1];
            vec3 nextCascade = cascadeContributions[i+1];
            
            float blendFactor = 0.08; // Reduced blending to prevent brightness accumulation
            cascadeGi = mix(cascadeGi, (prevCascade + nextCascade) * 0.5, blendFactor);
        }
        // Standard inter-cascade smoothing for other modes
        else if (i > 0 && i < (activeCascades - 1)) {
            vec3 prevCascade = cascadeContributions[i-1];
            vec3 nextCascade = cascadeContributions[i+1];
            
            float blendFactor = 0.08; // Subtle blending
            cascadeGi = mix(cascadeGi, (prevCascade + nextCascade) * 0.5, blendFactor);
        }
        
        float finalWeight = spatialWeight * betaWeight;
        indirectDiffuse += cascadeGi * finalWeight;
        totalWeight += finalWeight;
    }
    
    if (totalWeight > 0.0) {
        indirectDiffuse /= totalWeight;
    }
    
    // Apply additional spatial smoothing to the final GI result
    vec2 screenTexelSize = 1.0 / textureSize(gPosition, 0);
    vec3 smoothedIndirect = indirectDiffuse;
    
    // Light spatial smoothing pass over the final GI
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            if (x == 0 && y == 0) continue; // Skip center
            
            vec2 neighborCoord = TexCoords + vec2(x, y) * screenTexelSize;
            vec3 neighborPos = texture(gPosition, neighborCoord).xyz;
            // Reconstruct neighbor normal from RG16F format
        vec2 neighborNormalXY = texture(gNormal, neighborCoord).rg;
        float neighborNormalZ = sqrt(max(0.0, 1.0 - dot(neighborNormalXY, neighborNormalXY)));
        vec3 neighborNormal = normalize(vec3(neighborNormalXY, neighborNormalZ));
            
            // Only blend with similar geometry
            float depthDiff = abs(position.z - neighborPos.z);
            float normalSim = dot(normalize(normal), normalize(neighborNormal));
            
            if (depthDiff < 0.5 && normalSim > 0.8) {
                // Compute neighbor's GI contribution (simplified)
                vec3 neighborIndirect = vec3(0.0);
                for (int i = 0; i < 3; ++i) { // Only sample first 3 cascades for performance
                    vec4 neighborData = texture(rcTexture[i], neighborCoord);
                    neighborIndirect += neighborData.rgb * pow(0.8, float(i));
                }
                
                float blendWeight = 0.05; // Very subtle
                smoothedIndirect = mix(smoothedIndirect, neighborIndirect, blendWeight);
            }
        }
    }
    
    indirectDiffuse = smoothedIndirect;
    
    // Quality-aware indirect lighting scaling
    float qualityMultiplier = 0.4; // Base multiplier
    if (activeCascades >= 6) {
        // Ultra mode: Moderate reduction now that ambient/exposure are fixed
        qualityMultiplier = 0.22; // Moderate reduction for Ultra mode
    }
    indirectDiffuse *= ssgiStrength * qualityMultiplier;
    
    // Sample SSAO
    float ambientOcclusion = texture(ssaoTexture, TexCoords).r;
    
    // Energy conservation: direct + indirect should not exceed incoming light
    vec3 totalDiffuse = directDiffuse + indirectDiffuse;
    
    // Apply albedo to diffuse components only (both direct and indirect)
    vec3 diffuseContribution = totalDiffuse * albedo;
    
    // Add ambient term with SSAO applied
    vec3 ambient = ambientStrength * 0.05 * lightRadiance * albedo * ambientOcclusion; // Apply AO to ambient
    
    // Apply SSAO to indirect lighting for more realistic contact shadows
    indirectDiffuse *= mix(1.0, ambientOcclusion, ssaoStrength);
    diffuseContribution = (directDiffuse + indirectDiffuse) * albedo;
    
    // Combine final lighting: diffuse (with albedo and AO) + specular (without albedo) + ambient (with AO)
    vec3 finalColor = diffuseContribution + directSpecular + ambient;
    
    // FIXED: Apply exposure BEFORE tone mapping
    float exposure = 0.35; // Reduced base exposure
    finalColor *= exposure;
    
    // Simple Reinhard tone mapping for all modes (no Ultra special handling)
    finalColor = finalColor / (1.0 + finalColor);
    
    // Gamma correction
    finalColor = pow(finalColor, vec3(1.0/2.2));
    
    FragColor = vec4(finalColor, 1.0);
} 