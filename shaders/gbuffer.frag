#version 330 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec2 gNormal;
layout (location = 2) out vec4 gAlbedo;
layout (location = 3) out float gLinearDepth;
layout (location = 4) out vec2 gVelocity; // New: motion vector output
layout (location = 5) out vec3 gEmission; // New: emission output

in vec3 FragPos;
in vec3 Normal;
in vec3 ViewPos;
in vec3 ViewNormal;
in vec2 TexCoords;
in vec3 ViewTangent;
in vec3 ViewBitangent;
in vec2 Velocity; // From vertex shader

// Material uniforms
uniform vec3 objectColor;
uniform bool hasMaterial;
uniform vec3 materialBaseColor;
uniform float materialRoughness;
uniform float materialMetallic;
uniform float materialAO;
uniform vec3 materialEmission;

// Texture mapping uniforms
uniform vec2 materialTiling;
uniform float heightScale;

// Texture uniforms
uniform bool hasAlbedoMap;
uniform bool hasNormalMap;
uniform bool hasRoughnessMap;
uniform bool hasMetallicMap;
uniform bool hasAOMap;
uniform bool hasHeightMap;
uniform bool hasEmissionMap;

uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D roughnessMap;
uniform sampler2D metallicMap;
uniform sampler2D aoMap;
uniform sampler2D heightMap;
uniform sampler2D emissionMap;

vec3 getNormalFromMap(vec2 texCoords) {
    vec3 tangentNormal = texture(normalMap, texCoords).xyz * 2.0 - 1.0;
    
    vec3 N = normalize(ViewNormal);
    vec3 T = normalize(ViewTangent);
    vec3 B = normalize(ViewBitangent);
    mat3 TBN = mat3(T, B, N);
    
    return normalize(TBN * tangentNormal);
}

vec2 parallaxOcclusionMapping(vec2 texCoords, vec3 viewDir) {
    if (!hasHeightMap) {
        return texCoords; // No height map, return original coordinates
    }
    
    // Number of layers for parallax occlusion mapping
    const float minLayers = 8.0;
    const float maxLayers = 32.0;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));
    
    // Calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;
    
    // The amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy / viewDir.z * heightScale;
    vec2 deltaTexCoords = P / numLayers;
    
    // Get initial values
    vec2 currentTexCoords = texCoords;
    float currentDepthMapValue = texture(heightMap, currentTexCoords).r;
    
    while(currentLayerDepth < currentDepthMapValue) {
        // Shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // Get depthmap value at current texture coordinates
        currentDepthMapValue = texture(heightMap, currentTexCoords).r;
        // Get depth of next layer
        currentLayerDepth += layerDepth;
    }
    
    // Get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
    
    // Get depth after and before collision for linear interpolation
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(heightMap, prevTexCoords).r - currentLayerDepth + layerDepth;
    
    // Interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);
    
    return finalTexCoords;
}

void main()
{    
    // Store the fragment position in view space (more suitable for SSGI)
    gPosition = ViewPos;
    
    // Apply texture coordinate tiling
    vec2 tiledTexCoords = TexCoords * materialTiling;
    
    // Calculate view direction for parallax mapping
    vec3 viewDir = vec3(0.0, 0.0, 1.0); // Default tangent space view direction
    if (hasMaterial && hasHeightMap) {
        // Convert view direction to tangent space for parallax mapping
        vec3 viewDirWorld = normalize(-ViewPos); // View direction in view space
        vec3 N = normalize(ViewNormal);
        vec3 T = normalize(ViewTangent);
        vec3 B = normalize(ViewBitangent);
        mat3 TBN = mat3(T, B, N);
        viewDir = normalize(transpose(TBN) * viewDirWorld);
    }
    
    // Apply parallax occlusion mapping
    vec2 finalTexCoords = parallaxOcclusionMapping(tiledTexCoords, viewDir);
    
    // Calculate normal (either from normal map or vertex normal)
    vec3 normal;
    if (hasMaterial && hasNormalMap) {
        normal = getNormalFromMap(finalTexCoords);
    } else {
        normal = normalize(ViewNormal);
    }
    gNormal = normal.xy; // Store only X,Y - Z can be reconstructed
    
    // Calculate albedo (either from texture or material/object color)
    vec3 albedo;
    if (hasMaterial && hasAlbedoMap) {
        albedo = texture(albedoMap, finalTexCoords).rgb * materialBaseColor;
    } else if (hasMaterial) {
        albedo = materialBaseColor;
    } else {
        albedo = objectColor;
    }
    gAlbedo.rgb = albedo;
    
    // Store material properties in alpha channel (we'll need to expand this later)
    // For now, we'll use a simple encoding: roughness in alpha
    float roughness = hasMaterial ? materialRoughness : 0.5;
    if (hasMaterial && hasRoughnessMap) {
        roughness *= texture(roughnessMap, finalTexCoords).r;
    }
    gAlbedo.a = roughness;
    
    gLinearDepth = -ViewPos.z;
    
    // Store velocity for TAA
    gVelocity = Velocity;
    
    // Calculate emission (either from texture or material emission)
    vec3 emission = vec3(0.0);
    if (hasMaterial) {
        emission = materialEmission;
        if (hasEmissionMap) {
            emission *= texture(emissionMap, finalTexCoords).rgb;
        }
    }
    gEmission = emission;
} 