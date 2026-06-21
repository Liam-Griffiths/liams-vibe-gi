# Rendering Roadmap — toward a viable game engine

Goal: evolve this deferred + screen-space-GI renderer into a credible game engine.
Priorities below are ordered by **visual quality per unit of effort**. Do Tier 1 in order
before anything else — it fixes the shading base that every later system depends on.

## Architecture as it stands today (baseline)
- **Deferred renderer**, OpenGL 4.3+ (Linux/Win; macOS dropped).
- **G-buffer** (`shaders/gbuffer.frag`): `gPosition` (view-space pos), `gNormal`
  (octahedral RG16F), `gAlbedo` (rgb + **roughness in .a**), `gLinearDepth`, `gVelocity`,
  `gEmission`. **Metallic is computed but never written to the G-buffer.**
- **Lighting** (`shaders/final_composite.frag`): Lambert diffuse + a Phong-ish specular
  (`0.2 * pow(dot(viewDir,reflectDir),64)`). This is **not** metallic-roughness PBR.
- **GI**: screen-space radiance cascades (directionless merge) + bilateral post-filter.
  Collapses near walls (screen-space only). Partial mitigation: "Sky GI Fill" env-irradiance
  fallback in the composite.
- **Lights**: a single point/area light + one 4096² ortho shadow map.
- **Post**: SSAO, SSR (screen-space, roughness-gated, no off-screen fallback), TAA/FXAA,
  Reinhard tonemap @ hardcoded `exposure = 0.35`, gamma 2.2. No bloom.
- **Transparency**: forward screen-space-refraction pass (glass), runs after opaque composite.
- **Scenes**: ECS-style entities/components; glTF (cgltf) + OBJ loaders.

## Known concrete issues found while reading the code
- **Metallic dropped**: `gbuffer.frag` outputs roughness in `gAlbedo.a` only; the metallic
  uniform is set but never stored → metals render like grey plastic.
- **Not PBR**: composite lighting is Lambert + Phong (`final_composite.frag:271-281`).
- **Oct-normal decode mismatch**: `ssr.frag` decodes `gNormal` differently/incorrectly vs the
  full decode in `final_composite.frag` (skips the fold/remap) → reflections use wrong normals.
  Fix: one shared oct decode/encode used by all consumers.
- **TAA has no jitter**: `main.cpp` ~"No more jittering - clean, stable rendering" — TAA
  without sub-pixel jitter barely anti-aliases; cost paid for little gain.
- **Fixed internal resolution**: buffers hardcoded 1280×800 in places (`main.cpp` composite
  FBO, `RadianceCascades(1280,800,...)`) while the window can resize → stretch/clip.
- **Exposure hardcoded** 0.35; Reinhard desaturates highlights; no bloom.

---

## Tier 1 — Highest leverage (the shading base)

### 1. Real Cook-Torrance PBR + metallic in the G-buffer  ⭐ START HERE
- Add a metallic channel to the G-buffer (pack metallic+roughness, e.g. into a spare RG
  target or unused normal components).
- Replace composite lighting with **Cook-Torrance GGX**: Schlick Fresnel (F0 from metallic),
  GGX NDF, Smith geometry, energy-conserving diffuse/specular.
- Self-contained (G-buffer + composite shaders). Immediately visible on every existing scene.
- Unblocks correct SSR/GI (they become physically meaningful once F0/metallic exist).

### 2. Color pipeline + bloom + tonemapping
- **ACES or AgX** tonemap instead of Reinhard.
- **Bloom** (downsample → blur → upsample chain); pairs with existing emissive support.
- **Auto-exposure** (luminance histogram / eye adaptation) replacing the fixed 0.35.
- Audit albedo textures are **sRGB-decoded on load** (linear lighting requires linear input).

### 3. Image-Based Lighting (IBL)
- Prefiltered specular env cubemap + irradiance map, generated from `proceduralSky()`.
- Replaces the "Sky GI Fill" hack properly, gives **SSR an off-screen fallback**, and real
  reflections on the metals from #1. Reflection probes later.

## Tier 2 — Lighting architecture (what makes it an engine)

### 4. Many lights via clustered/tiled culling
- Move from one light to hundreds. Add light types: directional sun, point, spot.

### 5. Scalable shadows
- **Cascaded Shadow Maps** for the sun; cube/atlas shadows for point/spot.
- **Contact shadows** (short screen-space ray) for small-scale contact.

### 6. Fix TAA (and get temporal upscaling)
- Re-introduce Halton sub-pixel jitter + neighborhood-clamp resolve.
- Then render at 66–75% and temporally upscale to full res (cheap perf win).

## Tier 3 — GI & screen-space upgrades

### 7. World-space GI fallback
- Probe-based GI (**DDGI**) or world-space radiance cascades (note: a directional-cascade
  rewrite is already pending). Keep SSGI as the high-detail layer; probes fill off-screen.

### 8. SSAO → GTAO; unify + fix SSR
- GTAO for less haloing. Unify the oct-normal decode. Add IBL fallback for off-screen SSR.

## Tier 4 — Engine plumbing that gates quality at scale
- Make every render target track render resolution (kill the hardcoded 1280×800).
- GPU-driven rendering: instancing + frustum/occlusion culling + draw/material batching.
- Mesh LODs; material/shader sorting.

---

## Suggested execution order
Tier 1 (1 → 2 → 3) first and in order, then Tier 2, then 3–4. Each Tier-1 item makes the
later systems correct instead of fighting a broken base.
