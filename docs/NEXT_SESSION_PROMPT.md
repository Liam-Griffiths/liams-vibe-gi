/# Kickoff prompt for the next session

Copy everything in the block below into a fresh Claude Code session in this repo.

---

We're upgrading the renderer in this OpenGL 4.3 deferred engine. Read
`docs/RENDERING_ROADMAP.md` first — it has the full prioritized plan and a list of known
issues I found while reading the code.

**This session's task: Tier 1, item #1 — replace the fake lighting with real
Cook-Torrance PBR, and carry metallic through the G-buffer.**

Today the renderer is not actually PBR:
- `shaders/gbuffer.frag` writes roughness into `gAlbedo.a` but **never outputs metallic**
  (the metallic uniform is set but dropped), so metals look like grey plastic.
- `shaders/final_composite.frag` (around lines 271-281) lights with Lambert diffuse +
  a Phong-ish `0.2 * pow(dot(viewDir,reflectDir),64)` specular.

What I want:
1. Add a **metallic** channel to the G-buffer. Pick the cheapest correct packing (e.g. a new
   small target, or pack metallic+roughness together) and wire it through `gbuffer.frag`,
   the G-buffer setup in `src/main.cpp` / `src/RadianceCascades.cpp`, and the composite reads.
2. Replace the composite's direct lighting with a proper **Cook-Torrance GGX BRDF**:
   Schlick Fresnel with F0 derived from metallic (`mix(vec3(0.04), albedo, metallic)`),
   GGX normal distribution, Smith geometry term, energy-conserving diffuse (Lambert scaled by
   `(1 - F) * (1 - metallic)`) + specular. Keep the existing shadow/attenuation terms.
3. Keep the indirect/GI, SSAO, ambient, emission, tonemap stages working — only the direct
   lighting term and the material decode change.

Constraints / how I work:
- Match the surrounding code style; comment the *why*, not the *what*.
- Build with `cmake --build build -j4` after changes. The clang/LSP "file not found" and
  "undeclared identifier" diagnostics for imgui/GL headers are pre-existing include-path
  noise — ignore them; trust the cmake build.
- The default scene is glTF Sponza (run `scripts/fetch_sponza.sh` if models are missing).
  Scenes 0-7 in the dropdown; "A Beautiful Game" (7) has metals + glass for spot-checking.
- Don't commit or push unless I ask.

Two related issues you can fix opportunistically while you're in these shaders (both in the
roadmap): the **octahedral normal decode in `ssr.frag` is inconsistent with the one in
`final_composite.frag`** — unify them into one shared decode. Don't scope-creep beyond that.

When done, summarize what changed and suggest the next roadmap item (Tier 1 #2:
tonemapping/bloom/auto-exposure).

---
