# THE BORE COMMERCIAL (assets/trailer/bore_commercial.mp4)

A 65-second late-night TV spot cut from headless mGBA captures: the title, the acid menu, the creator, a day at home, THE CATALOG in the
builder, the dispensary, a cab to TACO BORE, the skate park, the pets, the TOUKEBOX, BORTUA FIGHTER's moon and the helium burst, then the
end card. Old-tube look: 4:3, scanlines, colour bleed, a rolling bar, jitter, barrel curve, vignette, channel-change static; the set clicks off.

How it is made:
1. `emu.c` is the headless runner (libmgba; `gcc -O2 -o emu emu.c -lmgba -lm`). Its script commands include `rec file.rgb` (every frame as
   raw RGB24 240x160 until `recstop`) and `wav file.wav` (the sound at 32768 Hz stereo).
2. The scene scripts (`*.txt`) run on a TEST build of the game: `hook_decl.c` / `hook_body.c` are spliced into main.c (before lifeStep and
   main) so a poke of `tstCall` runs a scene hook (6 the builder, 24 a cab to TACO BORE, 25 to its counter, 16/17 planters and a dispensary
   counter, 30 a skate course, 8 pets, 41 a teleport to tile x | y << 8 in `tstArg`). `@CALL@` / `@ARG@` are those two symbols' addresses
   (arm-none-eabi-nm on the test ELF). The fight scenes run on the fighter test build (bortua.h with the taunt forced: `@TSA@` / `@HOSEA@` pick
   the ending: 1 the slap, 2 the toot, 4 + 1 the burst through the back).
3. `python3 tools/make_trailer.py <capture dir> <out.mp4>` cuts it: the SCENES table holds the clip, its first frame, the seconds on screen
   and the cards (the jokes). ffmpeg muxes H.264 + AAC.
