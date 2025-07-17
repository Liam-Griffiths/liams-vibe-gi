#version 330 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec2 gNormal;
layout (location = 2) out vec4 gAlbedo;
layout (location = 3) out float gLinearDepth;
layout (location = 4) out vec2 gVelocity; // New: motion vector output

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

// Texture uniforms
uniform bool hasAlbedoMap;
uniform bool hasNormalMap;
uniform bool hasRoughnessMap;
uniform bool hasMetallicMap;
uniform bool hasAOMap;

uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D roughnessMap;
uniform sampler2D metallicMap;
uniform sampler2D aoMap;

vec3 getNormalFromMap() {
    vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;
    
    vec3 N = normalize(ViewNormal);
    vec3 T = normalize(ViewTangent);
    vec3 B = normalize(ViewBitangent);
    mat3 TBN = mat3(T, B, N);
    
    return normalize(TBN * tangentNormal);
}

void main()
{    
    // Store the fragment position in view space (more suitable for SSGI)
    gPosition = ViewPos;
    
    // Calculate normal (either from normal map or vertex normal)
    vec3 normal;
    if (hasMaterial && hasNormalMap) {
        normal = getNormalFromMap();
    } else {
        normal = normalize(ViewNormal);
    }
    gNormal = normal.xy; // Store only X,Y - Z can be reconstructed
    
    // Calculate albedo (either from texture or material/object color)
    vec3 albedo;
    if (hasMaterial && hasAlbedoMap) {
        albedo = texture(albedoMap, TexCoords).rgb * materialBaseColor;
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
        roughness *= texture(roughnessMap, TexCoords).r;
    }
    gAlbedo.a = roughness;
    
    gLinearDepth = -ViewPos.z;
    
    // Store velocity for TAA
    gVelocity = Velocity;
} 