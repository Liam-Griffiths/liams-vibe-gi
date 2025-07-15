#version 330 core
out vec3 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D gDepth;

uniform mat4 projection;
uniform mat4 view;
uniform float ssgiRadius;
uniform float ssgiIntensity;
uniform float ssgiMaxDistance;
uniform int ssgiSamples;
uniform vec2 screenSize;
uniform float time;

// Random number generation
float rand(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

// Generate hemisphere sample  
vec3 getHemisphereSample(vec3 normal, vec2 uv) {
    float phi = 2.0 * 3.14159265359 * uv.x;
    float cosTheta = sqrt(uv.y);
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    
    vec3 sample = vec3(
        cos(phi) * sinTheta,
        sin(phi) * sinTheta,
        cosTheta
    );
    
    // Create tangent space around normal
    vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, normal));
    vec3 bitangent = cross(normal, tangent);
    
    return normalize(tangent * sample.x + bitangent * sample.y + normal * sample.z);
}

// Sample nearby pixels for indirect lighting
vec3 sampleIndirectLight(vec3 position, vec3 normal, vec2 offset) {
    vec2 sampleCoord = TexCoords + offset * ssgiRadius / screenSize;
    
    // Check bounds
    if (sampleCoord.x < 0.0 || sampleCoord.x > 1.0 || 
        sampleCoord.y < 0.0 || sampleCoord.y > 1.0) {
        return vec3(0.0);
    }
    
    vec3 samplePos = texture(gPosition, sampleCoord).xyz;
    vec3 sampleNormal = texture(gNormal, sampleCoord).xyz;
    vec3 sampleAlbedo = texture(gAlbedo, sampleCoord).rgb;
    
    // Skip if no geometry
    if (length(sampleNormal) < 0.1) {
        return vec3(0.0);
    }
    
    // Calculate distance and direction
    vec3 lightDir = samplePos - position;
    float distance = length(lightDir);
    lightDir = normalize(lightDir);
    
    // Skip if too far
    if (distance > ssgiMaxDistance) {
        return vec3(0.0);
    }
    
    // Calculate lighting contribution
    float NdotL = max(dot(normal, lightDir), 0.0);
    float NdotL_sample = max(dot(sampleNormal, -lightDir), 0.0);
    
    // Distance attenuation
    float attenuation = 1.0 / (1.0 + distance * distance);
    
    // Return the indirect light contribution
    return sampleAlbedo * NdotL * NdotL_sample * attenuation;
}

void main()
{
    vec3 position = texture(gPosition, TexCoords).xyz;
    vec3 normal = texture(gNormal, TexCoords).xyz;
    vec3 albedo = texture(gAlbedo, TexCoords).rgb;
    
    // Skip background pixels
    if (length(normal) < 0.1) {
        FragColor = vec3(0.0);
        return;
    }
    
    vec3 gi = vec3(0.0);
    
    // Generate sample pattern
    vec2 noise = vec2(
        rand(TexCoords + time * 0.1),
        rand(TexCoords + time * 0.1 + 0.5)
    );
    
    int samples = min(ssgiSamples, 16); // Limit for performance
    
    // Sample in a circle pattern around the pixel
    for (int i = 0; i < samples; ++i) {
        float angle = 2.0 * 3.14159265359 * (float(i) + noise.x) / float(samples);
        float radius = (float(i) + noise.y) / float(samples);
        
        vec2 offset = vec2(cos(angle), sin(angle)) * radius;
        
        // Sample hemisphere directions
        vec2 hemisphereUV = vec2(
            (float(i) + noise.x) / float(samples),
            rand(TexCoords + float(i) * 0.1 + noise.y)
        );
        
        vec3 sampleDir = getHemisphereSample(normal, hemisphereUV);
        
        // Weight by cosine of angle with normal
        float weight = max(dot(normal, sampleDir), 0.0);
        
        vec3 contribution = sampleIndirectLight(position, normal, offset);
        gi += contribution * weight;
    }
    
    gi /= float(samples);
    gi *= ssgiIntensity;
    
    FragColor = gi;
} 