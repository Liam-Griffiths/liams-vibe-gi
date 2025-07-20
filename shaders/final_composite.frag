#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D gEmission; // New: emission texture for emissive materials
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

// Enhanced shadow calculation with distance-based softness and Poisson disk sampling
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, float lightDistance)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    float currentDepth = projCoords.z;
    
    // Improved bias calculation - more stable across different angles
    float bias = max(0.0005 * (1.0 - dot(normal, lightDir)), 0.0001);
    
    // Distance-based shadow softness - closer to light = softer shadows
    float shadowSoftness = clamp(lightDistance / lightRadius * 0.3, 0.5, 4.0);
    
    // Poisson disk sampling pattern for better shadow quality
    vec2 poissonDisk[32] = vec2[](
        vec2(-0.94201624, -0.39906216), vec2(0.94558609, -0.76890725),
        vec2(-0.094184101, -0.92938870), vec2(0.34495938, 0.29387760),
        vec2(-0.91588581, 0.45771432), vec2(-0.81544232, -0.87912464),
        vec2(-0.38277543, 0.27676845), vec2(0.97484398, 0.75648379),
        vec2(0.44323325, -0.97511554), vec2(0.53742981, -0.47373420),
        vec2(-0.26496911, -0.41893023), vec2(0.79197514, 0.19090188),
        vec2(-0.24188840, 0.99706507), vec2(-0.81409955, 0.91437590),
        vec2(0.19984126, 0.78641367), vec2(0.14383161, -0.14100790),
        vec2(-0.53028528, 0.54253327), vec2(-0.35838825, -0.23242493),
        vec2(0.37678673, 0.75146980), vec2(-0.61150398, -0.22013497),
        vec2(0.57493719, 0.31070804), vec2(-0.87863736, -0.14102842),
        vec2(0.23553348, -0.54442332), vec2(-0.04590084, 0.63188713),
        vec2(0.76693445, -0.33153748), vec2(-0.59069157, 0.61982369),
        vec2(0.10424420, 0.43456630), vec2(-0.12100214, -0.72732115),
        vec2(0.66884386, 0.44032156), vec2(-0.33883145, 0.85307121),
        vec2(0.29901922, -0.22748206), vec2(-0.77980003, 0.26770890)
    );
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    
    // Use more samples for higher quality
    int numSamples = 32;
    for(int i = 0; i < numSamples; ++i)
    {
        vec2 sampleCoord = projCoords.xy + poissonDisk[i] * texelSize * shadowSoftness;
        float pcfDepth = texture(shadowMap, sampleCoord).r;
        shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
    }
    shadow /= float(numSamples);
    
    // Soft edges when outside shadow map bounds
    if(projCoords.z > 1.0)
        shadow = 0.0;
    
    // Smooth transition at shadow map edges
    vec2 fadeDistance = smoothstep(0.0, 0.05, projCoords.xy) * (1.0 - smoothstep(0.95, 1.0, projCoords.xy));
    shadow *= fadeDistance.x * fadeDistance.y;
        
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

/**
 * Final Composite Shader with Enhanced Bilateral GI Post-Filter
 * 
 * This shader combines all lighting components (direct + indirect + ambient + specular)
 * into the final image with advanced bilateral filtering for ultra-smooth GI.
 */

// Bilateral GI post-filter for outlier removal and ultra-smooth results
vec3 bilateralGI(vec2 uv, vec3 centerGI) {
    vec3 centerPosition = texture(gPosition, uv).xyz;
    vec2 centerNormalXY = texture(gNormal, uv).rg;
    float centerNormalZ = sqrt(max(0.0, 1.0 - dot(centerNormalXY, centerNormalXY)));
    vec3 centerNormal = normalize(vec3(centerNormalXY, centerNormalZ));
    
    vec3 C = vec3(0.0);
    float W = 0.0;
    float sigmaN = 0.1; // normal weight - more tolerant for smoother results
    float sigmaD = 0.05; // depth weight - tight for edge preservation  
    vec2 invScreenRes = 1.0 / textureSize(gPosition, 0);
    
    // 5x5 bilateral kernel for better smoothing (as suggested)
    for(int y = -2; y <= 2; ++y) {
        for(int x = -2; x <= 2; ++x) {
            vec2 offset = vec2(float(x), float(y)) * invScreenRes;
            vec2 sampleUV = uv + offset;
            
            // Skip out-of-bounds samples
            if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0) {
                continue;
            }
            
            // Sample neighbor properties
            vec3 samplePosition = texture(gPosition, sampleUV).xyz;
            vec2 sampleNormalXY = texture(gNormal, sampleUV).rg;
            float sampleNormalZ = sqrt(max(0.0, 1.0 - dot(sampleNormalXY, sampleNormalXY)));
            vec3 sampleNormal = normalize(vec3(sampleNormalXY, sampleNormalZ));
            
            // Sample GI from the same cascade structure as the center pixel
            vec3 sampleGI = vec3(0.0);
            if (activeCascades > 0) {
                // Get the most detailed cascade (cascade 0) for bilateral filtering
                sampleGI = texture(rcTexture[0], sampleUV).rgb;
                
                // Blend with other cascades for completeness
                for (int c = 1; c < activeCascades; ++c) {
                    vec3 cascadeContrib = texture(rcTexture[c], sampleUV).rgb;
                    float blendFactor = 1.0 / float(c + 1); // Progressive blending
                    sampleGI = mix(sampleGI, cascadeContrib, blendFactor * 0.3);
                }
            } else {
                sampleGI = centerGI; // Fallback to center if no cascades
            }
            
            // Bilateral weights
            float dn = dot(centerNormal, sampleNormal);
            float dd = abs(centerPosition.z - samplePosition.z);
            
            // Combined weight calculation
            float spatialWeight = exp(-(float(x*x + y*y)) / 2.0); // Gaussian spatial weight
            float normalWeight = exp(-(1.0 - dn) / sigmaN);       // Normal similarity
            float depthWeight = exp(-dd / sigmaD);                // Depth similarity
            
            float w = spatialWeight * normalWeight * depthWeight;
            
            C += sampleGI * w;
            W += w;
        }
    }
    
    // Return filtered result or original if no valid samples
    return (W > 0.0) ? (C / W) : centerGI;
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
    
    float shadow = ShadowCalculation(fragPosLightSpace, worldNormal, lightDir, lightDistance);
    
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
        vec4 cascadeData = texture(rcTexture[i], TexCoords);
        cascadeContributions[i] = cascadeData.rgb;
        cascadeWeights[i] = cascadeData.a;
    }
    
    vec3 mergedGI = cascadeContributions[activeCascades - 1];
    for (int i = activeCascades - 2; i >= 0; --i) {
        float interpFactor = 1.0 - cascadeWeights[i];
        mergedGI = mix(mergedGI, cascadeContributions[i], interpFactor);
    }
    indirectDiffuse = mergedGI;
    
    // ULTRA-SMOOTH GI: Apply bilateral post-filter for final polish
    // This removes outliers and provides the smoothest possible result
    indirectDiffuse = bilateralGI(TexCoords, indirectDiffuse);
    
    // Universal spatial interpolation to "join up" sparse GI hits for smooth lighting
    vec3 originalGI = indirectDiffuse;
    
        // Apply to any scene with significant GI contribution (selective with higher res data)
    float giLuminance = dot(indirectDiffuse, vec3(0.299, 0.587, 0.114));
    
    if (giLuminance > 0.01) { // More selective threshold for higher quality data
        // Bilateral upsampling to smooth sparse GI lighting
        vec3 interpolatedGI = vec3(0.0);
        float interpolationWeight = 0.0;
        
        vec2 texelSize = 1.0 / textureSize(gPosition, 0);
        vec3 centerPosition = texture(gPosition, TexCoords).xyz;
        vec2 centerNormalXY = texture(gNormal, TexCoords).rg;
        float centerNormalZ = sqrt(max(0.0, 1.0 - dot(centerNormalXY, centerNormalXY)));
        vec3 centerNormal = normalize(vec3(centerNormalXY, centerNormalZ));
        
        // Large sampling pattern for interpolation
        vec2 interpolationSamples[12] = vec2[](
            vec2(-2.0, -2.0), vec2(0.0, -2.0), vec2(2.0, -2.0),
            vec2(-2.0,  0.0),                   vec2(2.0,  0.0),
            vec2(-2.0,  2.0), vec2(0.0,  2.0), vec2(2.0,  2.0),
            vec2(-3.0,  0.0), vec2(3.0,  0.0), vec2(0.0, -3.0), vec2(0.0,  3.0)
        );
        
        for (int i = 0; i < 12; ++i) {
            vec2 sampleCoord = TexCoords + interpolationSamples[i] * texelSize * 2.5;
            
            if (sampleCoord.x >= 0.0 && sampleCoord.x <= 1.0 && sampleCoord.y >= 0.0 && sampleCoord.y <= 1.0) {
                // Sample neighbor position and normal from G-buffer
                vec3 neighborPosition = texture(gPosition, sampleCoord).xyz;
                vec2 neighborNormalXY = texture(gNormal, sampleCoord).rg;
                float neighborNormalZ = sqrt(max(0.0, 1.0 - dot(neighborNormalXY, neighborNormalXY)));
                vec3 neighborNormal = normalize(vec3(neighborNormalXY, neighborNormalZ));
                
                // Sample neighbor GI from cascade 0 (highest quality)
                vec4 neighborData = texture(rcTexture[0], sampleCoord);
                vec3 neighborGI = neighborData.rgb;
                float neighborBeta = neighborData.a;
                float visibilityWeight = 1.0 - neighborBeta;
                
                // Bilateral weighting: similar depth and normal = higher weight
                float depthDiff = abs(centerPosition.z - neighborPosition.z);
                float normalDot = dot(centerNormal, neighborNormal);
                
                float depthWeight = exp(-depthDiff * 2.0); // Exponential falloff for depth
                float normalWeight = max(0.2, normalDot);  // Normal similarity
                
                // Distance weight for smooth falloff
                float distance = length(interpolationSamples[i]);
                float distanceWeight = exp(-distance * 0.3);
                
                // Combined weight
                float weight = depthWeight * normalWeight * distanceWeight * visibilityWeight;
                
                interpolatedGI += neighborGI * weight;
                interpolationWeight += weight;
            }
        }
        
        if (interpolationWeight > 0.0) {
            interpolatedGI /= interpolationWeight;
            
            // Blend original higher-res GI with interpolated smooth GI
            float blendAmount = 0.6; // 50% interpolated, 50% original for detail preservation
            indirectDiffuse = mix(originalGI, interpolatedGI, blendAmount);
        }
    }
    
    // Quality-aware indirect lighting scaling - increased for brighter GI
    float qualityMultiplier = 1.2; // Increased base multiplier for brighter scene
    if (activeCascades >= 6) {
        // Ultra mode: Still good quality but brighter than before
        qualityMultiplier = 0.8; // Increased for Ultra mode
    }
    indirectDiffuse *= ssgiStrength * qualityMultiplier;
    
    // Sample SSAO
    float ambientOcclusion = texture(ssaoTexture, TexCoords).r;
    
    // Energy conservation: direct + indirect should not exceed incoming light
    vec3 totalDiffuse = directDiffuse + indirectDiffuse;
    
    // Apply albedo to diffuse components only (both direct and indirect)
    vec3 diffuseContribution = totalDiffuse * albedo;
    
    // Add ambient term - increased for brighter scene
    vec3 ambientColor = vec3(1.0, 1.0, 1.0); // White ambient light
    // Reduce SSAO influence on ambient since ambient light is more diffuse
    float ambientAO = mix(1.0, ambientOcclusion, 0.3); // Only 30% SSAO influence
    vec3 ambient = ambientStrength * 4.0 * ambientColor * albedo * ambientAO; // Increased ambient multiplier
    
    // Apply SSAO to indirect lighting for more realistic contact shadows
    indirectDiffuse *= mix(1.0, ambientOcclusion, ssaoStrength);
    diffuseContribution = (directDiffuse + indirectDiffuse) * albedo;
    
    // Combine final lighting: diffuse (with albedo and AO) + specular (without albedo) + ambient (with AO)
    vec3 finalColor = diffuseContribution + directSpecular + ambient;
    
    // Add subtle direct emission contribution for surface visibility
    vec3 emission = texture(gEmission, TexCoords).rgb;
    
    // Apply exposure BEFORE adding emission and tone mapping
    float exposure = 0.35; // Reduced base exposure
    finalColor *= exposure;
    
    // Add minimal direct emission to make emissive surfaces visible (prevent double-counting)
    vec3 directEmission = emission * 0.15; // Very subtle direct contribution
    finalColor += directEmission;
    
    // Simple Reinhard tone mapping for all modes (no Ultra special handling)
    finalColor = finalColor / (1.0 + finalColor);
    
    // Gamma correction
    finalColor = pow(finalColor, vec3(1.0/2.2));
    
    FragColor = vec4(finalColor, 1.0);
} 