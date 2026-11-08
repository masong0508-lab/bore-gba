# BORE: GODGUY (Nintendo DS)

A god game spun off from BORE: shape a 128 x 128 world with the stylus, drop in life, and watch it live, eat, hunt and breed.

**v0.1 (this build):** world generator (continents, oceans, ice caps, deserts, jungles, swamps, lava), 12 ground types as generated tiles, 392 creature kinds (8 peoples + 24 animal archetypes x 16 colour families, every one with its own name, shape, diet, habitat and speed), a living ecosystem (grazing, hunting, breeding, starvation, old age, dirt regrowing into grass), and 21 divine powers (paint the ground, spawn, rain, fire, lightning, meteor, smite, heal, feed, inspect).

Controls: stylus = use the power; D-pad = pan (B faster); L / R = power; X / Y = brush size, or species while SPAWN is picked; SELECT = +10 species; A = pause; START = new world. Top screen: HUD and the whole-planet map.

Build: `make` with devkitARM + libnds. PC test of the world and the ecosystem: `gcc -O2 -DPCTEST -o pc pc.c && ./pc <seed> <years>` (prints populations, writes /tmp/gg_world.ppm and /tmp/gg_zoom.ppm).

Roadmap: villages and civilisations that build, wars, disasters over time, 300+ items and tools, 300+ enemies, the story mode that unlocks free-play items, bigger planets (the world arrays are the limit: 256 x 256 fits the DS's 4 MB), saves, then PSP / Vita / 3DS ports.
