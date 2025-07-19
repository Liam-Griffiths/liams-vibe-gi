#version 330 core
out vec4 FragColor;

in vec3 Normal;  
in vec3 FragPos;  
in vec4 FragPosLightSpace;
  
uniform vec3 lightPos; 
uniform vec3 viewPos; 
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform sampler2D shadowMap;

// Enhanced shadow calculation with distance-based softness and Poisson disk sampling
float ShadowCalculation(vec4 fragPosLightSpace)
{
    // Perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    // Get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // Get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    
    // Calculate bias (prevents shadow acne)
    vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float bias = max(0.0005 * (1.0 - dot(normal, lightDir)), 0.0001);
    
    // Distance-based shadow softness - closer to light = softer shadows
    float lightDistance = length(lightPos - FragPos);
    float shadowSoftness = clamp(lightDistance / 2.0 * 0.3, 0.5, 4.0); // Assume lightRadius = 2.0
    
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
    
    // Check whether current frag pos is in shadow
    // Use improved PCF (Percentage Closer Filtering) for softer shadows
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    int numSamples = 32;
    
    for(int i = 0; i < numSamples; ++i)
    {
        vec2 sampleCoord = projCoords.xy + poissonDisk[i] * texelSize * shadowSoftness;
        float pcfDepth = texture(shadowMap, sampleCoord).r;
        shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
    }
    shadow /= float(numSamples);
    
    // Keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
        shadow = 0.0;
    
    // Smooth transition at shadow map edges
    vec2 fadeDistance = smoothstep(0.0, 0.05, projCoords.xy) * (1.0 - smoothstep(0.95, 1.0, projCoords.xy));
    shadow *= fadeDistance.x * fadeDistance.y;
        
    return shadow;
}

void main()
{
    // ambient
    float ambientStrength = 0.15;
    vec3 ambient = ambientStrength * lightColor;
  	
    // diffuse 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;  
    
    // Calculate shadow
    float shadow = ShadowCalculation(FragPosLightSpace);       
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * objectColor;    
    
    FragColor = vec4(lighting, 1.0);
} 