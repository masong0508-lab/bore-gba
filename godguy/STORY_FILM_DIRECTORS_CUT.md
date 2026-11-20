# GODGUY — The Long Film Edition (3DS) / Story Edition (DS)

**Implementation status, 20 November 2026:** this document is an authored story treatment and production contract, not a claim that the complete feature-length film has been rendered. The current directed 3DS timeline has approximately 35 minutes of staged beats, plus baked opening footage; the DS shares roughly 21 minutes of staged beats, plus baked footage. Target: a coherent 65–80 minute 3DS cutscene narrative **after** cinematic motion, legibility, and performance tests pass. Do not pad shots to fake running time.

## What makes the film work

The story is not about Wisp automatically winning or the Hollow King being forgiven because a crown comes off. It is about people learning to take care of a world that can be harmed even by its protector. The recurring cup, bridge, roof, door, and ledger each have an actual physical location and change hands for a reason. A change of set requires a transition or a clear cut; a character cannot teleport across a conversation.

The DS gets a complete, understandable account with the same moral choices, clear enough to work in short text and simple cel animation. The 3DS adds supporting characters, consequences, reversals, additional quiet scenes, scene-relative lighting, real moving backgrounds and world-space choreography. **The 3DS version should be the director's cut, not simply a scaled or stereoscopic DS video.**

## Two new recurring characters (3DS film cast)

**NESSA — the bridgewright.** Arrives at the ford after a storm takes a boat; practical, skeptical of miracles, protective of travelers who cannot take the hill road. Her promise: build a crossing others can repair without her. Distinctive silhouette: dark braid, copper clasp, green working coat. Actions: tests footing, braces a beam, takes a hammer, passes a lamp to stranded families, trains volunteers, then walks away from a bridge that works.

**SERIN — the former royal surveyor.** Once drew the roads that made the Hollow King's siege possible. He knows where civilians hid, but he is afraid to confess his involvement. He cannot erase the maps; he must use them to locate survivors, document losses, and face the people he helped hurt. Distinctive silhouette: streaked gray hair, muted blue mantle, battered bronze badge. Actions: unfolds a real map, guides rescuers through a gate, hands the ledger to Elowen, confronts Hollow in public, keeps writing after the battle.

Neither is there solely to recite exposition. Both have goals, obstacles, changing choices, visible work, and final consequences.

## Directed acts — target running time

| Act | Chapters | Target 3DS story time | Dramatic purpose |
| --- | --- | --- | --- |
| I — A Light, Then a House | 1–3 | 12–14 min | Wisp learns that making a home is a commitment, not a gesture; Mira and Thorgrim build a dependable shelter |
| II — Across the River | 4–6 | 11–13 min | Nessa insists on engineering instead of salvation-by-speech; Serin learns where the army will strike |
| III — What the Records Hold | 7–9 | 11–13 min | Town grows, Elowen's ledger becomes a record of people, not monsters; friendship tested by the cost of growth |
| IV — The Fire and the Names | 10–12 | 15–18 min | Rescue through the gate and across the bridge; people act on information Serin once used to hurt them |
| V — The Empty Crown | 13–15 | 16–20 min | Hollow must listen to the dead's names, accept the right to be refused, and repair rather than rule |
| **Total target** | **15 films** | **65–78 min** | Narrative target; not yet the current measured runtime |

Shot count must be driven by action coverage, not artificial holds. The baseline already includes 15 chapter reels; add story beats and new acting keys only as they demonstrably advance a conflict or consequence.

## Ending — the strongest rewriting priority

1. **No sudden acquittal:** Hollow asks to help; Nessa does not trust him, but gives him a real task on the bridge under her direction. Nobody calls that forgiveness.
2. **The list is spoken aloud:** Serin and Elowen preserve the names and routes of those lost. Hollow has to hear individual names, not just a number.
3. **No magic reversal:** The film never pretends the dead return. The bridge may be repaired, but the witnesses remember.
4. **Passing responsibility on:** Nessa has trained others so the bridge holds when she goes home; the village can continue without one indispensable hero.
5. **The earned domestic ending:** Wisp knows what home means because people choose to leave a place for one another. Hollow comes in without a crown. The cup goes to the next thirsty traveler.

Avoid cheap comedy undercutting grief. Humor from Dave and the goat is welcome in recovery scenes, but do not cut from a named loss to a joke on the same beat.

## 3D location continuity and scene direction

| Place | Persistent geometry | Story-driven environmental change |
| --- | --- | --- |
| Shore and storm | Distant hills, near bank, tides and anchored trees | Moving water, rain falling at actual depth, first sprout |
| Village and hearth | Homes with roofs, doors, windows, beams, paths | One initial shelter, additional houses, animated workers, late communal light |
| Ford | The same riverbanks and stepping stones | Nessa's bridge appears after its engineering arc; rescue/repair scenes show danger and effort |
| Gate / ruins | Masonry, arches, traversable openings | Damage is spatially consistent; the rescue route matters |
| Forge / hall | Room walls, pillars, workbench and supporting surfaces | Hammer, lamp and records stay on their visible surfaces |
| Archive | Shelves, books, aisle and working desk | The ledger changes meaning as witnesses speak |
| Throne | Raised platform and a physical crown | Hollow's choice happens through action, not a giant zoom on the crown |

**Camera:** share a single world coordinate system between scenery, cast feet, supports, props and lens. An establishing track must change perspective at near and far depth by different amounts. Do not implement fake camera travel by digitally enlarging the same bitmap. Actors must face a reachable target before touching it.

**Animation:** schedule walk-in, anticipation, contact, force, recovery and reaction. Wind bends a tree about its trunk. Water moves in its plane. Flames and lamps illuminate a stable source. Tools stay gripped or grounded. No free-floating effects added merely to increase the number of changed pixels.

**Geometry:** prefer profiled roof sections, framed doors, beams, braces, stair/rail construction and arch details over plain cubes; the current software-rasterized model is the first geometry layer, not finished sculpting. 3DS is still rendering a 256×192 indexed film page, so higher-resolution native geometry/lighting and full 3D character meshes remain future work.

## Non-negotiable engineering acceptance

- DS and DSi must still build, render the shared rewritten dialogue and preserve their existing art/RAM budgets.
- 3DS has its own cinematic cast and world geometry, but must not allocate a z-buffer or second full-frame movie surface.
- All camera, character and prop coordinates must agree. No head-floating objects, behind-camera polygons, teleporting performers, mismatched cut continuity, or frame-to-frame arbitrary scenery relocation.
- Test both the 3DS standalone 3D geometry harness and full DS/3DS `pc_fmv.c` playback; inspect the real 20 fps movie reel rather than trusting static code.
- Verify the 3DS homebrew build, audio cadence and measured per-frame CPU cost on target devices before promising stable 20 fps. Do not equate a host benchmark to hardware FPS.
- Where a detailed geometry/acting change cannot meet the frame budget, simplify offscreen faces or distant details, not close actor silhouettes or the storyline.

## Immediate follow-up milestones

1. Verify this new renderer's build/test pipeline and inspect 3DS cinematic frames in an emulator; correct clipping or geometry occlusion before expanding scene complexity.
2. Move physical prop supports into the same 3D scene transform instead of keeping their legacy fixed screen coordinates.
3. Create additional 3DS-only acting/camera coverage for the five-act treatment, primarily the siege-to-finale arcs, until the runtime is **earned** and not static filler.
4. Upgrade from 2D character billboards to articulated 3D body models where the hardware budget permits, while retaining sharp close-up acting portraits and the DS cutscene fallback.
