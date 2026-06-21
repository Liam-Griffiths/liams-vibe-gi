#version 430 core
out vec4 FragColor;

in vec2 TexCoords;

// Per-cascade temporal resolve.
// Accumulates the raw (single-frame, noisy) GI estimate into a stable history using
// motion-vector reprojection plus a neighbourhood variance clamp. Splitting this out
// of the trace pass lets us read the current frame's spatial neighbourhood, which is
// what makes a real TAA-style clamp possible: history can keep accumulating *through
// camera motion* (killing blotchiness) without smearing across moving edges (ghosting).

uniform sampler2D currentGI;   // Raw GI traced this frame (no temporal)
uniform sampler2D historyGI;   // Accumulated GI from previous frame
uniform sampler2D gVelocity;   // Screen-space motion vector (UV delta)

uniform bool hasHistory;       // False on frame 0 / after a temporal reset
uniform int cascadeIndex;

void main() {
    vec4 cur = texture(currentGI, TexCoords);

    if (!hasHistory) {
        FragColor = cur;
        return;
    }

    // 3x3 neighbourhood statistics of the current (noisy) frame -> variance AABB.
    // Mean/stddev is more robust to a noisy current frame than raw min/max.
    vec2 texel = 1.0 / vec2(textureSize(currentGI, 0));
    vec4 m1 = vec4(0.0);
    vec4 m2 = vec4(0.0);
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            vec4 s = texture(currentGI, TexCoords + vec2(x, y) * texel);
            m1 += s;
            m2 += s * s;
        }
    }
    vec4 mean = m1 / 9.0;
    vec4 sigma = sqrt(max(vec4(0.0), m2 / 9.0 - mean * mean));
    float gamma = 1.25; // clamp width in std-devs; wider = smoother but more ghosting
    vec4 nmin = mean - gamma * sigma;
    vec4 nmax = mean + gamma * sigma;

    // Reproject history. Reject samples that reproject off-screen (disocclusion).
    vec2 velocity = texture(gVelocity, TexCoords).xy;
    vec2 prevUV = TexCoords - velocity;
    bool valid = all(greaterThanEqual(prevUV, vec2(0.0))) &&
                 all(lessThanEqual(prevUV, vec2(1.0)));
    if (!valid) {
        FragColor = cur;
        return;
    }

    vec4 hist = texture(historyGI, prevUV);
    // Variance clamp keeps the reprojected history inside the current local range so
    // it cannot smear across moving silhouettes.
    vec4 clampedHist = clamp(hist, nmin, nmax);

    // Low blend factor => strong accumulation that persists during motion (steadier GI
    // while moving). Lower values = more stable but slower to settle. Cascade 0 reacts a
    // little faster since it carries the highest-frequency detail.
    float alpha = (cascadeIndex == 0) ? 0.07 : 0.05;
    FragColor = mix(clampedHist, cur, alpha);
}
