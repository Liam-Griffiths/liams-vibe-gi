#version 430 core
// Screen-space refraction for glass / alpha-blended materials. Samples the already-rendered
// opaque scene (backgroundTex) behind the fragment, offset along the refracted view ray, and
// tints/blurs it by the material. Output is opaque (alpha 1) - the "see-through" is faked by
// reading the background, so hardware depth sorts glass against glass correctly.
out vec4 FragColor;

in vec3 ViewPos;
in vec3 ViewNormal;

uniform sampler2D backgroundTex; // opaque scene color (frozen copy)
uniform sampler2D gPosition;     // opaque view-space positions, for occlusion test
uniform vec2 invResolution;

uniform mat4 view;
uniform vec3 lightPos;           // world space
uniform vec3 lightColor;
uniform vec3 viewPosCam;         // camera world pos (unused dir; kept for parity)

// Material
uniform vec3 baseColor;
uniform float roughness;
uniform float transmission;
uniform float opacity;
uniform float ior;
uniform float refractScale;      // GUI: strength of the background distortion

void main()
{
    vec2 uv = gl_FragCoord.xy * invResolution;

    // Occlusion against opaque geometry: gPosition stores opaque view-space position (z<0).
    // If this fragment is farther than the opaque surface in front of it, it's hidden.
    // sceneZ ~ 0 means background/sky (no geometry) - never occlude there.
    float sceneZ = texture(gPosition, uv).z;
    if (sceneZ < 0.0 && ViewPos.z < sceneZ - 0.01) discard;

    vec3 I = normalize(ViewPos);              // camera->fragment (view space, eye at origin)
    vec3 N = normalize(ViewNormal);
    if (dot(N, I) > 0.0) N = -N;              // face the normal against the incident ray

    // --- Refraction offset: bend the view ray and sample the opaque scene behind ---
    float eta = 1.0 / max(ior, 1.0001);
    vec3 R = refract(I, N, eta);
    // Scale the screen-space offset by surface depth so distant glass distorts less.
    vec2 offset = R.xy * refractScale / max(-ViewPos.z, 0.5);

    // Roughness-driven blur: a few taps spread by roughness (frosted glass).
    float blur = roughness * 0.04;
    vec3 bg = vec3(0.0);
    const int TAPS = 5;
    vec2 dirs[5] = vec2[](vec2(0.0), vec2(1.0,0.0), vec2(-1.0,0.0), vec2(0.0,1.0), vec2(0.0,-1.0));
    for (int i = 0; i < TAPS; ++i) {
        vec2 s = clamp(uv + offset + dirs[i] * blur, vec2(0.001), vec2(0.999));
        bg += texture(backgroundTex, s).rgb;
    }
    bg /= float(TAPS);

    vec3 transmitted = bg * baseColor;        // tint the background by glass color

    // --- Surface shading for the non-transmitted part ---
    vec3 lightView = (view * vec4(lightPos, 1.0)).xyz;
    vec3 L = normalize(lightView - ViewPos);
    float NdotL = max(dot(N, L), 0.0);
    vec3 litSurface = baseColor * (0.15 + NdotL * lightColor);

    // How much we see through: transmission (glass) or low opacity (alpha blend).
    float seeThrough = clamp(max(transmission, 1.0 - opacity), 0.0, 1.0);
    vec3 col = mix(litSurface, transmitted, seeThrough);

    // --- Fresnel edge reflection (approx, no env map) ---
    float F0 = 0.04;
    float fres = F0 + (1.0 - F0) * pow(1.0 - max(dot(N, -I), 0.0), 5.0);
    vec3 reflColor = vec3(0.7) * lightColor;  // cheap sky/key approximation
    col = mix(col, reflColor, fres);

    // --- Specular highlight from the main light (Blinn-Phong) ---
    vec3 V = -I;
    vec3 H = normalize(L + V);
    float shininess = mix(8.0, 128.0, 1.0 - roughness);
    float spec = pow(max(dot(N, H), 0.0), shininess);
    col += spec * lightColor;

    FragColor = vec4(col, 1.0);
}
