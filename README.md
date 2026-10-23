# BORE

A Game Boy Advance game about one voxel creature and the life it leads. You build the creature, give it a home, and then live: eat, work, skate, make friends, fall in love, raise a family, get into the odd fight. It started as a creature creator and grew into a life sim in the spirit of The Sims, with a skateboard.

The view is isometric and pseudo-3D. The CPU draws every pixel and no libraries are used.

## Contents
[Get the game](#get-the-game) · [First ten minutes](#first-ten-minutes) · [Controls](#controls) · [The main menu](#the-main-menu) · [Living your life](#living-your-life) · [Skating](#skating) · [Fighting](#fighting-and-weapons) · [Out and about](#out-and-about) · [Expansion packs](#expansion-packs) · [Building](#building) · [Neighborhoods](#neighborhoods) · [The creator](#the-creator-and-share-codes) · [Music](#music) · [Options](#options) · [Saving](#saving) · [For developers](#for-developers)

## Get the game
**From GitHub.** Push to the repository and the *Build GBA ROM* workflow makes the ROMs. Download them from the run's artifacts, or from the *Latest build* release, which every push to `main` updates.

**Locally.** Install devkitARM and Python 3, then run `make`. Play `bore.gba` in mGBA or on a flash cart.

What you get:
- `bore.gba` is the whole game, every expansion pack included. This is the one to play.
- `bore-base.gba` is the base game alone.
- `packs/BORE-EP*.bps` are the six expansion packs as BPS patches. Apply them to `bore-base.gba` in any order, alone or together; all six give `bore.gba` byte for byte. A pack that is not in shows NOT INSTALLED on the shelf.
- `make creator` builds `bore-creator.gba`, the **Creator Demo**: only the creature creator, in the spirit of Spore's Creature Creator. Every part and slider is free, you can keep Bores in MY CREATIONS, and each one has a share code.

Saves use the cart's 128 KB of flash (58 slots). A cart without flash falls back to 32 KB of SRAM (12 slots). Older saves carry over.

## First ten minutes
1. Press START on the title screen, then choose **Play**.
2. Choose **New Game**. You can make a Bore in the creator, move in a ready-made family, roll a random Sim, or start a Story Mode.
3. The town map opens. Pick a home lot and buy it (your starting cash is shown against each price).
4. The first time you play, the game offers a short tutorial that walks you through moving, running, hopping, the board, the fridge and the pause menu. It can be replayed from OPTIONS.
5. Look after the bars on screen. Press **R** at the fridge when you are hungry, at the toilet, in bed, at the shower. A thought bubble over your head shows what is most urgent.
6. Open the phone and choose **CAREER** to pick a job. Skating is the default one, and it pays you for the tricks you land.
7. Press **START** to pause, check **MY SIM** to see how you are doing, and **SAVE** before you stop.

## Controls
**On foot**

| Button | What it does |
|---|---|
| D-pad | Walk (the D-pad always means up and down the screen, however the view is turned) |
| B (hold) | Run |
| A | Hop over low things |
| L | Step on or off the skateboard (you need to own one) |
| R | Use what is beside you: fridge, bed, toilet, shower, sofa, phone, TV, a Sim, a pet. With nothing near, it opens **YOUR ACTIONS** |
| START | Pause menu |
| SELECT (tap) | Switch to another Sim in the household |
| SELECT + L / R | Turn the view a quarter turn and keep it |
| SELECT + UP / DOWN | Zoom |

**On the board**

| Button | What it does |
|---|---|
| D-pad | Steer |
| A (hold) | Push to gain speed |
| B | Ollie. A tap is a hop, holding gives full height |
| In the air: LEFT / RIGHT | Spin |
| In the air: A (hold) | Grab |
| In the air: B | Kickflip (UP + B is a heelflip) |

**Fighting** (on foot, teen or older): hold **L** to guard, **L + R** to take a fight stance, then **R** to punch and **A + R** to kick. A weapon in hand turns R into its attack. More under [Fighting](#fighting-and-weapons).

**Menus**: D-pad moves, **A** picks, **B** goes back, **L / R** change tab. Every screen prints its own button hints along the bottom. The creator, the builder and the options screens have extra controls, listed in their sections below.

## The main menu
| Entry | What it is for |
|---|---|
| **Play** | Your life. Pick a town, then CONTINUE the life you saved, VISIT the town, or start a NEW GAME there |
| **Create a Bore** | The creature creator |
| **Build Mode** | The room builder: walls, floors, wallpaper and furniture |
| **Toukebox** | The jukebox and music player |
| **Room Slots** | Save and load rooms, houses, people and households |
| **Options** | Video, gameplay, time, audio, controls and more |
| **Expansions** | The shelf of expansion packs, and the extra modes some of them add |
| **?** | HOW TO PLAY, the in-game manual |

## Living your life
**Needs and mood.** Your Bore gets hungry, needs the toilet, tires, gets grubby, wants comfort, a pleasant room and company. Furniture does the work: the fridge feeds, the toilet, bed, shower and sofa each fill their need. Neglect has a price: a full bladder ends in an accident, an empty stomach in a faint. Fun comes mostly from skating. A bored or sad Bore is slower and scores fewer trick points, and a stoked one is faster and scores more. Food and a night in bed restore health.

**Wants and fears.** Every Sim carries a few wants and fears. Meeting a want raises the aspiration meter and earns reward points; a fear coming true pulls it down. Wants met close together build a streak. A want nobody meets for a few minutes is swapped for a new one unless you lock it. Each aspiration also has a lifetime want. You will find all of this under **MY SIM**.

**Time.** A game day lasts six real minutes by default (OPTIONS > TIME changes it, or stops the clock).

**Work and money.** Open the phone and choose CAREER. There are nine tracks, from Entertainment and Slacker to Business, Military, Education, Fast Food (the part-time job teens can take) and the default, **Pro Skater**. A normal job counts the minutes you spend up and about at your work marker during a shift; the Pro Skater job counts trick points instead. A good shift pays and builds towards a promotion, which also wants a skill and, higher up, friends. Bad shifts earn strikes. Bills fall due every midnight, so keep cash in hand. Teens and adults work; children go to school on weekdays; elders retire.

**Skills and rewards.** Cooking, logic, body, charm and creativity grow by doing: cooking at the fridge, practising in YOUR ACTIONS, and so on. Skating has its own skills. Reward points are spent in the REWARD SHOP (MY SIM > STUFF). **Jenes** (DNA) come from living and unlock creator parts and slider packs.

**YOUR ACTIONS.** Press R with nothing near you. A ring of choices opens: BODY (stretch, push-ups, flex), MIND (meditate, practise a speech, doodle, daydream), FUN (dance, sing, air guitar, have a cry) and CARE (freshen up, cat nap, deep breath). Each costs a little time and builds a skill or a need.

**People.** Walk up to a Sim and press R for the social menu: talk, joke, compliment, hug, flirt, kiss and on to going steady, proposing and having a baby, or the unfriendly choices. Sims form friendships and rivalries on their own, so the household and the neighbours get on with their lives while you get on with yours. Up to fourteen Sims share a household, and SELECT lets you switch to any of them and play their life. Households grow up: weddings, babies, children and elders. The pause menu's **FAMILY** shows who lives here, how they feel and how they are related.

**Pets.** The phone's ADOPT A PET opens the pet creator for dogs and cats. Pets have their own needs and moods. Press R next to one to pet, cuddle, play, feed, house-train or teach it tricks. Untrained pets will occasionally have accidents.

**Life stages.** Bores grow from baby to child, teen, adult and elder. Each stage has its own limits, and OPTIONS > TIME slows ageing down or switches it off.

**Death and trouble.** You can be hurt, and in the default setting you can die. OPTIONS > PLAY has a gentler HURT setting and a no-death one. The cops notice crime, and a busted Sim goes to **prison**, where there is a yard, work, rep, gangs, visits from the household, and a fence to climb if you dare.

## Skating
Skating is the heart of the game, and it is also the default job.

1. **Get a board.** Nobody starts with one now. Buy one at the **Skate Shop** (a complete board or a pro deck), or find one: every park, skate park, plaza and old town hides a board in its most out-of-the-way spot, and it glints when you are within two tiles.
2. **Step on it** with L. Boards are for houses, parks and plazas; shops and other indoor places say NO SKATING IN HERE.
3. **Push, steer, ollie.** Ramps, kickers, rails, ledges, benches and halfpipes are all in the town, and you can build your own.
4. **Land clean.** Spin and flip in the air, but be facing the way you are travelling when you touch down, or you bail and get hurt.
5. **Chain tricks into combos.** Grind rails and ledges, tap or ride walls by holding R as you hit one, and hold R on a manual pad. Switch tricks score more. A combo is banked when it ends cleanly.
6. **Fill the SPECIAL meter** under the score. A full meter doubles your points.
7. **Look for goals.** Every lot has a hidden tape, the letters S-K-A-T-E, a score goal and a lot-cleared bonus. PAUSE > MY SIM > MORE > VIEW GOALS tracks them, and TIMED RUN is a two-minute high-score attempt with a medal.

## Fighting and weapons
- **Hold L** to guard. Raise it just as a blow lands and it is a **parry**: no damage, and your next hit is harder.
- **L + R** takes a fight stance. Then **R** throws a jab, a cross and a hook (the finisher). **A + R** is a longer, slower kick. Keep the beat and the combo grows.
- **Special moves** work in the stance: a fireball (down, down + side, side, then R), an uppercut (side, down, down + side, then R) and a spin kick (down, release, down, then R). Landed blows, parries and knock-outs fill the SPECIAL meter that pays for them.
- **Weapons** (bat, knife, taser, pistol, UZI, rocket) come only from the ARMS SHOP lot, or from secret stashes hidden in the town's community lots. With one in hand, **R attacks**; hold R and tap L to change weapon. Nobody is armed in prison.

## Out and about
- **CALL A CAB** on the phone takes you anywhere in the town, and lists the kinds of places your town does not have yet. It will find land and build one.
- **Shops and venues** each have their own room and counter: the Skate Shop, Taco Bore, a gym, an arcade, a nightclub, a grocery, a bank, a hospital, a cafe, a bar, a salon, a pet shop, a library, a casino, a cinema, a pool. Roughly sixty-five things to do there, from eating and working shifts to haircuts, bets and getting patched up.
- **The green.** For grown-ups (teens only if the option is on): the GROW PLANTER (BUY > HOME) takes seeds, water and time. You can smoke, sell to neighbours or at the legal dispensary, and the cops take an interest in the other kind of selling.
- **The phone** also invites people over, calls people over, orders pizza or Chinese food, and moves someone out.
- **The TV**, radio and sound system are things to use with R. The TV plays short clips. The radio and sound system tune the jukebox by genre.

## Expansion packs
The **Expansions** shelf on the main menu (and STORY in the pause menu) holds the base game's ten missions and six packs. A **mission** is a short scene played inside your own life: a title, two lines of what is going on, up to three tasks that tick off in any order, and a payout. A pack's missions open one after another. Missions never touch your own lot's contents while you are away on an arena.

| Pack | What it is |
|---|---|
| **The base game** | Ten missions, one for each thing the game does: build, skate, the town, romance, a career, pets, fights, the neighbours and the green |
| **BORE-OUT MAN!** | Five missions. A burnt-out desk jockey becomes an astronaut: the spaceport, a launch, a spacewalk |
| **BOREZIES!** | Five missions told as a five-chapter story on six arenas (the cul-de-sac, the Winchester, the lab, the arcade, the scrapyard, the car park). The town has turned into the bored undead; fun and fists calm them. Finish all five and **HORDE MODE** opens: 100 waves or endless, fourteen mutators, a perk every five waves, saved records |
| **WORLD WAR BORE** | Ten missions. The East Side has come to trim your hedge: boot camp, the arms race, soldiers on your lawn |
| **BORTUA FIGHTER** | Five missions and **ARCADE MODE**: one-on-one best-of-three fights up a ladder of eight to the chrome boss BORAL. Also at the arcade cabinet and the dojo |
| **BOREagotchi!** | Fifteen missions and **THE HANDHELD**: a pixel blob on a keychain, plus the real pets. Feed it, clean it, scold it, put the light out at night |
| **BOREpilogue** | Twenty missions, grown-ups only: retire, remember, settle scores, leave a legacy |

**Story Mode** (New Game > Story Mode) is separate: Roommates, Newlyweds and Single Parent each set you six chapters, and each chapter pays cash and Jenes.

## Building
Enter the builder with **Build Mode** on the main menu, with EDIT MAP in the pause menu's BUILD page, or with BUILD on a lot in the town.

- **Tools** (SELECT changes tool): ROOM, WALL, FLOOR, BUY and SELL. A places, B cancels. SELECT + L / R changes wallpaper, SELECT + UP / DOWN changes floor, and START opens the map menu.
- **Undo and redo.** SELECT + B undoes and SELECT + START redoes. A step is everything between pressing A and letting go, and the last three are kept.
- **Floors.** A house can have three floors joined by stairs. Floors can be open to the sky.
- **BUY** is a shop of looks. Everything you place keeps working as the thing it is: a catalog chair is still sat on, a catalog bed is still slept in. The catalog has close to three hundred items in tabs: chairs, sofas, beds, tables, desks, storage, lights, plants, kitchen, bath, tech, art, outdoor, kids, skate, grown-ups, seasonal, and the rest, up to the **COUNTERS** tab.
- **Counters with a job.** In the COUNTERS tab (and the kitchen's own sink and microwave stand), R at a sink washes you up and R at a microwave heats a quick snack. A fridge counter is a fridge.
- **Build items.** Doors, windows, floors, wallpaper (some of it see-through fences), and pool, beach, garden, tree and yard items. Pick water in the FLOOR tool and it digs a **pool**; walk in and you swim, slower and cleaner, and the board stays out.
- **Skate pieces.** Rails, ledges, benches, kickers, quarter pipes, halfpipes, funboxes and long ramps.
- **Flags.** Place a COMMUNITY FLAG (visitors come and go) or a SKATE FLAG (a skater starts there) on a lot to turn it into a park, skate park or both.
- **Community lots** are visited rather than built while you stand in them. Build one from the town view (A on the lot, then BUILD). MAKE COMMUNITY and MAKE RESIDENTIAL switch a lot between the two, as long as it is not your home and has no household.

## Neighborhoods
A town is a map of lots. Choose one under **Play**, or make your own.

- **A on a lot** offers Play, Build, Move In and Rename. Moving in buys the lot and sells your old home, priced by land and what is built on it.
- **Shape the land** with the town tools (L / R change tool, SELECT picks a lot's kind): paint ground, lay roads, plant trees and decorations, and add lots. A lot can be a home, a park, a skate park, a plaza, a lounge or part of the old town.
- **START opens the town menu**: zoom, season, time of day, rename, grow or shrink the town, all neighborhoods, leave. SHRINK THE TOWN warns you before it deletes anything.
- Towns come from small to huge, and new ones fill themselves with districts and venues.

## The creator and share codes
Open it with **Create a Bore**. A live preview sits in a small room. Tabs: **BODY**, **FACE**, **HAIR**, **CLOTHES**, **PARTS**, **ASPIRE**, **DONE**. L / R change tab, UP / DOWN pick a row, LEFT / RIGHT change it, SELECT turns the creature and START jumps to DONE.

Body shape and sliders, life stage, faces, hair and beards, hats and clothes are all yours. **Parts** (tails, horns, spikes, wings, cat parts) are more than decoration: horns hit harder, spikes soften a blow, wings help you dodge and glide. Big parts cost Jenes. **Aspire** sets the aspiration, lifetime want, star sign and traits (25 points to share). On **DONE** you can roll the dice for a random Bore, start your life, or add Bores to the household and set who is related to whom.

**The Wattersons** (Gumball, Darwin, Anais, Nicole and Richard) are a ready-made family you can start as or move in.

**Share codes** turn a Bore into up to about 72 letters and digits, such as `AG9E FK8H W7R8 ...`. The alphabet has no I, O, 0 or 1, and a mistyped code is refused, not loaded as someone else.
- In the demo: hub > ENTER A SHARE CODE, or a creation's START in MY CREATIONS shows its code.
- In the game: DONE > SHARE CODE or ENTER A CODE (the Bore becomes the one on screen), or DONE > HOUSEHOLD > ADD FROM A CODE (the Bore joins your household).
- A Bore with a cat part gets a `C` code. Every other Bore keeps its `A` / `B` code, and old codes still load.

## Music
**Toukebox** is a music player with a NOW PLAYING card (song, artist, elapsed and total time, progress bar) above one list showing every song's length.

| Button | What it does |
|---|---|
| A | Play a song (A on the playing song stops it) |
| L / R | Previous / next song |
| SELECT | Check or uncheck a song. Only checked songs are chosen at random |
| START | Play mode: Shuffle, In order, Repeat (saved) |
| LEFT / RIGHT | Volume |

- Menus play GOTTCHO BARRACHO by default (OPTIONS > AUDIO > MENU MUSIC). GAME MUSIC, which plays random checked songs while you live, is off by default.
- A radio or sound system tunes by genre: House and Dance, Breaks and Bass, Chill and World, Latin, Rock and Prog, Hip Hop, or All Songs.
- A few songs are locked until a Sim meets their lifetime want, and songs named `... (ORIGINAL)` are hidden. The title code (UP UP DOWN DOWN LEFT LEFT RIGHT B A START) shows both.
- Creator music is twelve chiptune loops played live from note data.

**Adding songs.** Tracker song: put the `.xm` in `tools/`, add a `SONG_XM(...)` line to `source/songs.h` (and a name in `source/artists.h`), run `python3 tools/xm2gba.py` (needs numpy and scipy), then commit `source/musicdata.h` and `source/music/xmdata.bin`. Streamed song: `python3 tools/encode_song.py "song.mp3"` (needs ffmpeg). The list holds up to 64; songs added at the end keep everyone's checkmarks. To lock a song, add a line to `source/unlocks.h`.

## Options
Eight pages, changed with L / R: **VIDEO** (presets, frame rate, zoom, performance info), **PLAY** (needs, jobs, bills, score, hurt level, free will), **TIME** (day length, clock, ageing), **AUDIO**, **INPUT** (swap buttons, button test), **HUD**, **ROOMS** and **DATA**. A gold dot marks anything you changed, SELECT puts a row back, and RESET ALL OPTIONS on the DATA page undoes everything, so feel free to experiment.

## Saving
- **SAVE** in the pause menu keeps your life and house. Each player has a save file of their own; the town is shared. AUTO SAVE LIFE is in OPTIONS > PLAY.
- **Room Slots** keep rooms (one floor), houses (all three), people (a Bore with its life, cash and clock) and households.
- Slots hold 58 on flash and 12 on SRAM. DATA > SAVE MEMORY TEST checks that your cart or emulator really keeps saves.

## For developers
| Path | What |
|---|---|
| `source/main.c` | The game: screens, creator, life, audio mixer |
| `source/*.h` | One module each: sims, households, fight, venues, catalog, and so on |
| `source/songs.h`, `artists.h`, `musicdata.h` | Song list, artists, converted music |
| `source/music/` | Music data (`xmdata.bin`) and streamed songs |
| `tools/` | Song converters, music generators, art and catalog scripts |
| `docs/NOTES.md` | Full technical notes: memory maps, save layout, every feature |
| `docs/NEXT.md` | What to build next |

Things you will want to know:
- **Items and the catalog.** `tools/make_catalog.py` generates the catalog tables and art, and `sh tools/bake_items.sh` bakes sprites into ROM. New looks are always appended last, so saved rooms never change.
- **Options.** Add one at the end of the `XO_` enum, its count and default, a read where the game uses it, and an `XR(...)` row in `source/optscreen.h` (see the top of `source/opts.h`).
- **Missions.** One `XM(...)` row in a pack's table; see `source/expand.h`.
- **In-game help.** The pages are the `*Help[]` arrays (`lifeHelp`, `creatureHelp`, `mapHelp`, `nbHelp`, `packHelp`, `jbHelp`, `slotHelp`, `optHelp`) shown by `howToPlay()`. Keep each line under about 52 characters.
- **The commercial.** `assets/trailer/bore_commercial.mp4` is a 65-second late-night TV spot cut from the game itself (`tools/make_trailer.py`, pipeline in `tools/trailer/README.md`). Rated M for munchies.
