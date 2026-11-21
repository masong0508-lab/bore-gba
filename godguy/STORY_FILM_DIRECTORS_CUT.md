# GODGUY — The Long Film Edition (3DS) / Story Edition (DS)

**Screenplay implementation status, 20 November 2026:** the 3DS cinematic timeline is now scripted for **84 minutes 48 seconds of campaign-directed shots**: 34:44 of shared/existing extended scenes plus 375 newly authored eight-second beats (50:00). Six independent, replayable, chapter-unlocked anthology films add **exactly 40:00** of scenes (300 additional eight-second beats), for **124 minutes 48 seconds of authored, directed screenplay** plus the original baked movie footage. The DS/DSi retains the stronger shared 20:44 directed scenes. All totals describe **scheduled scene lengths in source**, not an emulator-verified or fully rendered two-hour movie. Realistic acting pace, native 3DS character geometry, animation, audio mix, old-hardware FPS, and complete film playback still require verification and polish.

**Measured host playback:** the completed GitHub Actions screenplay/playback regression reports **86:27 campaign including baked scenes**, **40:00 unlockable anthology**, and **2:06:27 combined scheduled film timeline**. This is a verified sequence clock, not a claim that every screen has final full-body 3D character models or certified hardware FPS.

## What makes the film work

The story is not about Wisp automatically winning or the Hollow King being forgiven because a crown comes off. It is about people learning to take care of a world that can be harmed even by its protector. The recurring cup, bridge, roof, door, and ledger each have an actual physical location and change hands for a reason. A change of set requires a transition or a clear cut; a character cannot teleport across a conversation.

The DS gets a complete, understandable account with the same moral choices, clear enough to work in short text and simple cel animation. The 3DS adds supporting characters, consequences, reversals, additional quiet scenes, scene-relative lighting, real moving backgrounds and world-space choreography. **The 3DS version should be the director's cut, not simply a scaled or stereoscopic DS video.**

## Two new recurring characters (3DS film cast)

**NESSA — the bridgewright.** Arrives at the ford after a storm takes a boat; practical, skeptical of miracles, protective of travelers who cannot take the hill road. Her promise: build a crossing others can repair without her. Distinctive silhouette: dark braid, copper clasp, green working coat. Actions: tests footing, braces a beam, takes a hammer, passes a lamp to stranded families, trains volunteers, then walks away from a bridge that works.

**SERIN — the former royal surveyor.** Once drew the roads that made the Hollow King's siege possible. He knows where civilians hid, but he is afraid to confess his involvement. He cannot erase the maps; he must use them to locate survivors, document losses, and face the people he helped hurt. Distinctive silhouette: streaked gray hair, muted blue mantle, battered bronze badge. Actions: unfolds a real map, guides rescuers through a gate, hands the ledger to Elowen, confronts Hollow in public, keeps writing after the battle.

Neither is there solely to recite exposition. Both have goals, obstacles, changing choices, visible work, and final consequences.

## Directed acts — implemented script schedule

The 15 feature-chapter arrays in `source/story_feature_3ds.h` add 25 purpose-written eight-second shots to each existing 3DS episode. Every new beat has a named location, camera, physical performers and/or an authored action. The main arc reaches **84:48** of staged material; the older baked prologues add additional time.

| Act | Chapters | Main conflict |
| --- | --- | --- |
| I — A Light, Then a House | 1–3 | Wisp learns why creation requires maintenance; Mira opens the house, Thorgrim makes the roof others can repair |
| II — Across the River | 4–6 | Nessa refuses shortcuts and teaches engineers; Serin chooses to name his complicity rather than hide behind orders |
| III — What the Records Hold | 7–9 | The hall debates scarcity, Elowen turns the ledgers into names, and Nessa learns to share responsibility |
| IV — The Fire and the Names | 10–12 | The siege rescue moves along a real gate-to-ford route; survivors refuse false reassurances and the community ages |
| V — The Empty Crown | 13–15 | Dave offers human-scale respite; Hollow hears names, accepts refusal, carries material responsibility and asks for a task |

### The six unlockable films — 40 additional minutes

Access: **Extras → Film Archive** on the 3DS only. Unlocks follow save-compatible chapter progress; completed films can be replayed without advancing a save.

| Film | Unlock after | Running time | Dramatic core |
| --- | --- | --- | --- |
| The First Winter | Chapter 3 | 6:40 | Shared shelter, food scarcity and Wisp's limits |
| The Bridge in Winter | Chapter 5 | 6:40 | Nessa and Gruul risk a structural failure and choose to close the crossing |
| The Map That Lied | Chapter 8 | 6:40 | Serin actively accepts he erased a school from the survey |
| The Room With No Throne | Chapter 11 | 6:40 | Dave refuses to make Hollow's wrongdoing into a joke or instant absolution |
| Names at Dawn | Chapter 14 | 6:40 | Elowen and Elder Mira leave space for those not ready to remember publicly |
| After the Crown | Campaign completion | 6:40 | Hollow respects refusal and chooses ordinary, accountable work |

**Calculated totals:** 15×25×8 seconds = 50:00 newly staged campaign material, plus 34:44 existing staged campaign = **84:48**; six×50×8 seconds = **40:00** bonus; combined staged schedule = **2:04:44** before the pre-rendered film portions. If it feels like padded dialogue, the right response is to write and animate more purposeful coverage—not simply increase shot durations.

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

**Geometry:** prefer profiled roof sections, framed doors, beams, braces, stair/rail construction and arch details over plain cubes; the current software-rasterized model is the first geometry layer, not finished sculpting. 3DS currently renders a 256×192 indexed film page. **Humanoid actors still use high-detail 2D cels projected into 3D sets; they are not yet polished skinned 3D models.** The acceptance standard and explicit anti-bubble-head requirements are in [CHARACTER_FIDELITY_BIBLE.md](3ds/CHARACTER_FIDELITY_BIBLE.md).

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
3. Review and improve the authored 84:48 campaign and 40:00 bonus timeline shot by shot; give every silent beat physical action, lip timing, reaction and continuity. Revise unnatural holds rather than hiding them in the runtime total.
4. Upgrade from 2D character billboards to articulated 3D body models where the hardware budget permits, while retaining sharp close-up acting portraits and the DS cutscene fallback.
