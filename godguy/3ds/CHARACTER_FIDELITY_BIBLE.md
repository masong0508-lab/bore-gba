# GODGUY 3DS — Character Fidelity and Animation Bible

**Quality bar:** The 3DS movie must feature clean, intentional, well-proportioned characters, not primitive bubble-headed figures, unshaded spheres, distorted blobs, floating body parts or awkwardly scaled handheld props.

**Current implementation:** Main cinematics have 40×48 detailed, twelve-pose indexed-color 2D acting cels placed within projected 3D world-space sets. These are a **temporary, legible quality floor**—not final 3D character models. Do not claim the 3D characters are finished simply because the backgrounds are geometric.

## 1. Visual identity and anatomy

- **Mira:** Adult human-like proportions, approximately seven heads tall, balanced shoulders, clearly defined neck, rib cage, pelvis, thighs and calves. Warm, expressive eyes recessed into the face; a recognizable braid and practical outer layers. Hands actually close around handles.
- **Nessa:** Taller working-build silhouette, broader back from physical labor, braid and worn coat with an asymmetric copper clasp. Her face must remain distinguishable from Mira in neutral lighting, front view and silhouette. A lean under load comes from the pelvis and spine, not from moving her head independently.
- **Serin:** Older, long-faced surveyor with brow, jaw, narrow cheeks, worn mouth and a distinct streak of gray hair. Not a masked generic guard. Map, quill and satchel must have persistent attachment points and visible weight.
- **Thorgrim:** Compact, substantial working torso, stable foot plant, thick forearms, shaped beard and helmet. His mallet/hammer aligns to his grip, wrist and the surface it strikes.
- **Gruul:** Powerful but articulated build. Clavicles/shoulders, elbows, knees and ankles are distinguishable from his costume; avoid round disconnected limbs.
- **Elowen:** Deliberate posture and subtle facial acting, book/pen hand posing; wardrobe has clear seams and readable layering. Avoid a generic floating face.
- **Hollow:** Gaunt, upright, asymmetrical and imposing without an oversized sphere for a skull. His identity should remain legible after the crown and cloak are removed; posture changes as his power changes.
- **Dave:** Lived-in clothing, relaxed but properly connected shoulder/hip anatomy and expressive hands. Seated animation must make believable contact with the couch and floor.
- **Wisp:** A tiny, intentionally nonhuman luminous spark. Its size must never be inflated to a human-head balloon. Give it directional intent, angular wing-like shape and a coherent emissive body—not a random moving particle cloud.
- **Elder Mira:** An aged person, not the original body scaled down or given gray hair only. Stable, grounded walk cycle; posture and expression shaped by years.

**Avoid:** chibi/bubble-head ratios for the humanoid cast, perfectly round cheeks/eyes, disconnected cylinder arms, spherical torsos, flattened billboards sold as finished geometry, tiny feet, random costume noise, or facial expressions based solely on enlarging the whole head.

## 2. Mesh, animation and look-dev requirements

All numeric targets are *starting budgets requiring hardware measurement*, not claims of current capability.

| Asset class | Candidate triangles | Notes |
| --- | ---: | --- |
| Hero character, near camera | 1,200–2,400 | Prioritize shaped face profile, hands, torso/hip connection and costume |
| Medium background performer | 400–900 | Preserve recognisable silhouette and gait |
| Distant crowd role | 80–250 | Avoid eye and finger geometry that cannot resolve in pixels |
| Main skeleton | 18–24 major joints | Stable feet and hands; few moving bones where possible |
| Distant skeleton | 8–12 major joints | Share walk cycles, avoid per-frame allocations |

**No extra full framebuffer, floating-point mesh solve for every screen pixel, or per-frame asset construction.** Prefer an eventual hardware-assisted 3DS renderer only if it demonstrably fits memory, frame time, and build constraints. The current scene software renderer is not yet a final skinned-mesh pipeline. Preserve its illustrated-cel fallback until the hardware path has passed visual and performance tests.

- Silhouette first: tapered jaw, neck under chin, tapered joints, shaped shoulders, rib cage and hips. Faces must work in 3/4 angle without a texture having to fake every feature.
- Material discipline: at most a few purposeful material regions per costume—fabric, metal, skin, hair, emissive magic. Avoid glossy doll skin and featureless plastic specular.
- Texture quality: compact indexed or shared atlases, clean edges and deliberate shading; avoid baked fuzzy interpolation, oversized noisy dithering and texture swimming.
- Face performance: eyes look *at* the partner; brows and eyelids communicate doubt, pain, attention and subtle humor. Mouth opening must end when the line ends.
- Rig continuity: turning starts from weight shift and shoulders, not detached head rotation. Walks use correct opposing arm/leg phases, planted feet without sliding, and a stable eye line.
- Real object interaction: anticipation, reachable grip, contact, transfer, release and aftermath. A cup rests on a sill, a hammer head meets a nail, a quill contacts a page. The camera and actors use the same three-dimensional coordinates.
- Cinematography: stable 3D world, physically motivated dolly and blocking, no bitmap zoom as a substitute for movement. Near and far objects travel at different speeds under camera motion.

## 3. Lighting and spatial clarity

Aim for a confident hand-authored cinematic look. Strong readable shape design takes priority over simulated realism. Use believable facial planes (brow, nose bridge, cheeks, jaw) and broad light groups that follow each scene's sun, fire or lamp. Keep cast colors stable across cuts. Shadows should correspond to the foot or body area touching the ground.

The 3DS version is the definitive visual treatment, but its superiority must come from the sets, acting, camera and geometry—not from the console's stereoscopic display alone. A player with 3D depth turned off must still see clean film-quality staging.

## 4. Shot-by-shot acceptance tests

Before replacing the temporary 2D actor cels, capture real Old 3DS and New 3DS output at native screen size:

1. Character turntable: front, both three-quarter views, profile and rear. No collapse into a bubble head, face texture smear, or shapeless torso.
2. Neutral and expressive closeups of Mira, Serin, Nessa, Hollow. Every face must remain recognizable at the **actual output resolution**, not only on a PC render.
3. Full-body moving shots: approach camera, turn around, stoop, pick up cup, carry beam, sit on couch, reach for quill. Check foot contact, arm joints, shadows and weight.
4. Ensemble dialogue: two actors face each other without mirrored faces or swapped screen sides. Speech animates for line duration and settles into a reaction.
5. Environmental response: when an actor passes behind a bridge rail, the rail truly occludes the correct body parts. Rain/water/light remain attached to the set.
6. Frame-budget and RAM comparison with and without hardware character meshes on Old 3DS and New 3DS, sound running. If targets are missed, reduce invisible geometry and distant LOD *before* simplifying faces and close shots.
7. DS regression: none of these new 3DS-only assets or cast arrays may change DS/DSi ROM assets, memory usage, dialogue timing or controls.

**Do not publish a low-quality provisional bubble-head mesh as the new default.** Keep the intentional 2D actor art visible until the high-quality 3D pass meets the standards above.

## 5. Main feature and unlockable film scope

The 3DS campaign adds 15 original 25-beat scenes (375 × 8 seconds = 50 minutes) on top of the established main and expanded reels, bringing the **authored staged campaign** above 80 minutes before baked intro footage. The archive contains six original 50-beat films (300 × 8 seconds = 40 minutes). The combined scripted timeline is **more than 120 minutes**.

This is a measured *script timing* contract, **not** verification that the final 3D presentation is 120 minutes of polished, naturally paced animation. Every additional scene must earn its screen time and pass dialogue legibility, coherent blocking and performance review.
