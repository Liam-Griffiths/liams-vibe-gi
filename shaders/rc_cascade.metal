#include <metal_stdlib>
using namespace metal;

// Vertex shader input structure
struct VertexIn {
    float2 position [[attribute(0)]];
    float2 texCoord [[attribute(1)]];
};

// Vertex shader output / Fragment shader input
struct VertexOut {
    float4 position [[position]];
    float2 texCoord;
};

// Uniform buffer structure
struct Uniforms {
    float4x4 view;
    float4x4 projection;
    float4x4 invView;
    float4x4 invProjection;
    float4 lightDirection;
    float4 lightColor;
    float4 cameraPosition;
    float2 screenSize;
    float time;
    int cascadeIndex;
    float cascadeSpacing;
    float angularResolution;
    float temporalWeight;
    int frameIndex;
};

// Vertex shader for fullscreen quad
vertex VertexOut vertex_main(VertexIn in [[stage_in]]) {
    VertexOut out;
    out.position = float4(in.position, 0.0, 1.0);
    out.texCoord = in.texCoord;
    return out;
}

// Radiance Cascades computation fragment shader
fragment float4 fragment_main(VertexOut in [[stage_in]],
                             constant Uniforms& uniforms [[buffer(0)]],
                             texture2d<float> gPosition [[texture(0)]],
                             texture2d<float> gNormal [[texture(1)]],
                             texture2d<float> gAlbedo [[texture(2)]],
                             texture2d<float> prevCascade [[texture(3)]],
                             texture2d<float> temporalHistory [[texture(4)]],
                             sampler textureSampler [[sampler(0)]]) {
    
    float2 texCoord = in.texCoord;
    float2 pixelCoord = texCoord * uniforms.screenSize;
    
    // Sample G-buffer
    float3 worldPos = gPosition.sample(textureSampler, texCoord).xyz;
    float3 normal = normalize(gNormal.sample(textureSampler, texCoord).xyz);
    float3 albedo = gAlbedo.sample(textureSampler, texCoord).rgb;
    
    // Skip background pixels
    if (length(worldPos) < 0.001) {
        return float4(0.0, 0.0, 0.0, 1.0);
    }
    
    // Calculate cascade parameters
    float cascadeDistance = uniforms.cascadeSpacing * pow(2.0, float(uniforms.cascadeIndex));
    float3 viewPos = (uniforms.view * float4(worldPos, 1.0)).xyz;
    float distanceFromCamera = length(viewPos);
    
    // Skip pixels outside this cascade's range
    float minDistance = cascadeDistance * 0.5;
    float maxDistance = cascadeDistance * 2.0;
    if (distanceFromCamera < minDistance || distanceFromCamera > maxDistance) {
        return float4(0.0, 0.0, 0.0, 1.0);
    }
    
    // Initialize radiance accumulation
    float3 radiance = float3(0.0);
    float weight = 0.0;
    
    // Direct lighting contribution
    float3 lightDir = normalize(-uniforms.lightDirection.xyz);
    float NdotL = max(dot(normal, lightDir), 0.0);
    float3 directLight = uniforms.lightColor.rgb * uniforms.lightColor.w * NdotL;
    radiance += directLight * albedo;
    weight += 1.0;
    
    // Indirect lighting from previous cascade or environment
    if (uniforms.cascadeIndex > 0) {
        // Sample from previous cascade for indirect illumination
        float3 sampleColor = prevCascade.sample(textureSampler, texCoord).rgb;
        
        // Apply angular sampling for this cascade level
        int angularSamples = max(8, int(uniforms.angularResolution / pow(2.0, float(uniforms.cascadeIndex))));
        
        for (int i = 0; i < angularSamples; ++i) {
            float angle = (float(i) / float(angularSamples)) * 2.0 * M_PI_F;
            float3 sampleDir = float3(cos(angle), sin(angle), 0.0);
            
            // Transform sample direction to world space
            float3 tangent = normalize(cross(normal, float3(0.0, 1.0, 0.0)));
            float3 bitangent = cross(normal, tangent);
            float3 worldSampleDir = tangent * sampleDir.x + bitangent * sampleDir.y + normal * sampleDir.z;
            
            // Sample radiance in this direction
            float2 sampleOffset = worldSampleDir.xy * cascadeDistance * 0.1;
            float2 sampleTexCoord = texCoord + sampleOffset / uniforms.screenSize;
            
            if (sampleTexCoord.x >= 0.0 && sampleTexCoord.x <= 1.0 && 
                sampleTexCoord.y >= 0.0 && sampleTexCoord.y <= 1.0) {
                float3 indirectSample = prevCascade.sample(textureSampler, sampleTexCoord).rgb;
                float cosine = max(dot(worldSampleDir, normal), 0.0);
                radiance += indirectSample * albedo * cosine * (1.0 / float(angularSamples));
                weight += cosine * (1.0 / float(angularSamples));
            }
        }
    } else {
        // Cascade 0: Add ambient lighting
        radiance += albedo * 0.1;
        weight += 0.1;
    }
    
    // Normalize by accumulated weight
    if (weight > 0.0) {
        radiance /= weight;
    }
    
    // Temporal accumulation
    if (uniforms.frameIndex > 0) {
        float3 prevRadiance = temporalHistory.sample(textureSampler, texCoord).rgb;
        float temporalBlend = uniforms.temporalWeight;
        radiance = mix(prevRadiance, radiance, temporalBlend);
    }
    
    // Apply exposure and tone mapping
    radiance = radiance / (radiance + 1.0); // Simple Reinhard tone mapping
    
    return float4(radiance, 1.0);
}

// Copy shader for final output
fragment float4 fragment_copy(VertexOut in [[stage_in]],
                             texture2d<float> inputTexture [[texture(0)]],
                             sampler textureSampler [[sampler(0)]]) {
    return inputTexture.sample(textureSampler, in.texCoord);
}

// Blur shader for temporal smoothing
fragment float4 fragment_blur(VertexOut in [[stage_in]],
                             constant Uniforms& uniforms [[buffer(0)]],
                             texture2d<float> inputTexture [[texture(0)]],
                             sampler textureSampler [[sampler(0)]]) {
    
    float2 texelSize = 1.0 / uniforms.screenSize;
    float3 result = float3(0.0);
    
    // 5x5 Gaussian blur
    float weights[5] = {0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216};
    
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            float2 offset = float2(float(x), float(y)) * texelSize;
            float weight = weights[abs(x)] * weights[abs(y)];
            result += inputTexture.sample(textureSampler, in.texCoord + offset).rgb * weight;
        }
    }
    
    return float4(result, 1.0);
} 