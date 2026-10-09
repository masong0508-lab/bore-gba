// songs.h - THE JUKEBOX SONG LIST. Order here = the "IN ORDER" order. Up to 64 songs.
//
//   SONG_XM(id,"NAME","file")    a tracker song (.xm). id = any unique C name, file = the .xm in tools/. After adding one, run
//                                python3 tools/xm2gba.py   to rebuild source/musicdata.h and source/music/xmdata.bin (the title music, THE DIPPER MAN, is one too)
//   SONG_ADP(id,"NAME","file")   a streamed ADPCM song. id = any unique C name, NAME = capitals, digits, spaces, punctuation and the extended Latin letters
//                                (no double quote or backslash; see "EXTENDED FONT" in README.md)
//
// Easiest way to add a song:   python3 tools/encode_song.py "my song.mp3"
// It writes source/music/<id>.adp and appends the SONG_ADP line below for you.
//
// HIDDEN SONGS: a song whose NAME ends in " (ORIGINAL)" (the old version of a song you reworked) or starts with PLACEHOLDER is hidden from the jukebox, the menu music
// and the game music until the code UP UP DOWN DOWN LEFT LEFT RIGHT B A START is entered on the title screen. Nothing else to do: just name it that way.
//
// Song spec: 4-bit IMA-ADPCM, mono, 18157 Hz (the game's music rate), about 9 KB per second of music.
// When this list changes size the saved shuffle is re-rolled automatically (next boot).

SONG_XM(the_dipper_man,"THE DIPPER MAN","tools/the_dipper_man.xm")

// ---- PLACEHOLDERS (short test tunes so shuffle / select can be heard now) ----
// Hidden in the jukebox unless the title-screen code UP UP DOWN DOWN LEFT LEFT RIGHT B A START is entered (any song named PLACEHOLDER...).
// Delete these 3 lines and the 3 files in source/music/ when you add your real songs.
SONG_ADP(placeholder_a,"PLACEHOLDER A","source/music/placeholder_a.adp")
SONG_ADP(placeholder_b,"PLACEHOLDER B","source/music/placeholder_b.adp")
SONG_ADP(placeholder_c,"PLACEHOLDER C","source/music/placeholder_c.adp")

// ---- songs added by tools/encode_song.py go below this line ----
SONG_XM(earth_and_the_space_citizens,"EARTH AND THE SPACE CITIZENS","tools/earth_and_the_space_citizens.xm")
SONG_XM(amiga_music,"AMIGA MUSIC","tools/amiga_music.xm")
// EMERGENCY ON THE DANCE FLOOR: the hi-tech rework (tools/make_emergency_rework.py). The earlier version is the secret one below.
SONG_XM(emergency_hitech,"EMERGENCY ON THE DANCE FLOOR","tools/emergency_hitech.xm")
// the earlier rework (xm2gba.py's make_dance): a SECRET song, hidden until the title-screen code (isDbgSong in main.c)
SONG_XM(emergency_dance_floor,"EMERGENCY (ORIGINAL)","tools/emergency_dance_floor.xm")
// TREE-AGE IN ACTION (TREE SWAYING ACTION until it was renamed): the ambient version (xm2gba.py make_tree_eno): a semitone down, no drums, generated from the song's own pad, pluck and
// bass, about 13 minutes ending in a slow fade. The drum rework with the breeze echo is the secret TREE-AGE IN ACTION (ORIGINAL) at the end.
SONG_XM(tree_swaying_action,"TREE-AGE IN ACTION","tools/tree_swaying_action.xm")
SONG_XM(meltdown_in_mars_house,"MELTDOWN IN MARS","tools/meltdown_in_mars_house.xm")
SONG_XM(worthless_clouds,"WORTHLESS CLOUDS","tools/worthless_clouds.xm")
SONG_XM(sunman_sunrise,"SUNMAN SUNRISE","tools/sunman_sunrise.xm")

// GOTTCHO BARRACHO: the rework (tools/make_barracho_rework.py), the sister song of AN ODE TO THE SPANISH FLEXICODE.
SONG_XM(gottcho_barracho_ii,"GOTTCHO BARRACHO","tools/gottcho_barracho_ii.xm")
// GOTTCHO BARRACHO v1 = the first full transcription of Dipper - Borracho (from the isolated stems). A SECRET song: hidden from the
// jukebox until the code UP UP DOWN DOWN LEFT LEFT RIGHT B A START is entered on the title screen (isDbgSong in main.c).
SONG_XM(gottcho_barracho,"GOTTCHO BARRACHO (ORIGINAL)","tools/gottcho_barracho.xm")
// AN ODE TO THE SPANISH FLEXICODE: a soft 16-channel rework of "The Dipper Man - An Ode to Mexicode" (tools/the_dipper_man_ode_to_mexicode.xm),
// built by tools/make_flexicode_rework.py.
SONG_XM(spanish_flexicode,"AN ODE TO THE SPANISH FLEXICODE","tools/spanish_flexicode.xm")

// MI CORA ZONE = a rework of THE DIPPER MAN (133 BPM, 92 bars, about 2:50): intro, drop, hook, breakdown, second drop,
// half-time bridge, key lift (+2 semitones) and outro. Built from the original sounds; the generator is not in the repo.
SONG_XM(mi_cora_zone,"MI CORA ZONE","tools/mi_cora_zone.xm")
// EXCUSES: an expansive house / ambient rework of "The Dipper Man - Excuses" (tools/make_excuses_rework.py, about 6:47)
SONG_XM(excuses_house,"EXCUSES","tools/excuses_house.xm")
// WHISTLER MAN: a steely half-time shuffle (the Purdie shuffle) rework of "The Dipper Man - Whistler Man" (tools/make_whistler_rework.py, about 5:37)
SONG_XM(whistler_shuffle,"WHISTLER MAN","tools/whistler_shuffle.xm")
// WHISTLER MAN (ORIGINAL): the first version of the rework, kept as a SECRET song, hidden until the title-screen code (isDbgSong in main.c)
SONG_XM(whistler_shuffle_old,"WHISTLER MAN (ORIGINAL)","tools/whistler_shuffle_old.xm")
// THE CYNICAL SYNDICATION (once CYNICALLER MADNESS): a 172 BPM liquid / tech drum & bass rework of "The Dipper Man - Cynicaller Madness" on 64th-note rows
// (tools/make_cynicaller_rework.py, about 4:05)
SONG_XM(cynicaller_dnb,"THE CYNICAL SYNDICATION","tools/cynicaller_dnb.xm")
// HOT DAMN: a 90s rave / IDM rework (breakbeats, acid, hoover, a Moby-style breakdown, drill'n'bass) of the Caustic project
// "HoTdamn v050" (tools/hotdamn_v050.caustic, read by tools/make_hotdamn_rework.py, about 3:48)
SONG_XM(hotdamn_rave,"HOT DAMN","tools/hotdamn_rave.xm")
// HOT DAMN (ORIGINAL): the first, busier take, kept as a SECRET song (isDbgSong in main.c)
SONG_XM(hotdamn_rave_old,"HOT DAMN (ORIGINAL)","tools/hotdamn_rave_old.xm")
// AIM AND SHOOT: the Caustic sketch "Aimandshoot 430 2021 V11" grown into a full piece in C# Hijaz: oud, qanun, ney and strings on a
// half-time groove with a quiet darbuka (tools/aimandshoot_v11.caustic, read by tools/make_aimandshoot_rework.py, about 2:52)
SONG_XM(aim_and_shoot,"AIM AND SHOOT","tools/aim_and_shoot.xm")
// THE MAGIC ACT: a Singhs rework of "The Dipper Man - The Magic Act" (tools/the_dipper_man_the_magic_act.xm), built by tools/make_magicact_rework.py.
// A suite in thirteen parts (about 6:47) where the original's arpeggio and hook keep coming back in a new mode, meter and colour.
SONG_XM(magic_act,"THE MAGIC ACT","tools/magic_act.xm")
// NURSERY TIME: a Singhs rework of "The Dipper Man - Nursery Time" (tools/the_dipper_man_nursery_time.xm), built by tools/make_nursery_rework.py.
// A music-box ditty that turns into a long, shifting metal / nu-groove suite built from the same tune (about 6:08).
SONG_XM(nursery_time,"NURSERY TIME","tools/nursery_time.xm")
// TREE-AGE IN ACTION (ORIGINAL): the drum rework (make_tree + the smooth / gated breeze echo), kept as a SECRET song (isDbgSong in main.c)
SONG_XM(tree_swaying_action_old,"TREE-AGE IN ACTION (ORIGINAL)","tools/tree_swaying_action.xm")
// TREE-AGE IN ACTION (FAST): the fast Breno Orêian rework in E flat, transcribed from the samples by pitch detection (tools/make_tree_fast_rework.py, from tools/tree_swaying_action.xm)
SONG_XM(tree_fast,"TREE-AGE IN ACTION (FAST)","tools/tree_fast.xm")
// TREE-AGE IN ACTION (FAST) (ORIGINAL): the shorter first version (2:02, tools/make_tree_fast_old_rework.py), a SECRET song (isDbgSong in main.c)
SONG_XM(tree_fast_old,"TREE-AGE IN ACTION (FAST) (ORIGINAL)","tools/tree_fast_old.xm")
// CONDENSED MUSIC: a Sk9m UK garage rework of The Dipper Man - Condensed Music (tools/make_condensed_rework.py, from tools/the_dipper_man_condensed_music.xm)
SONG_XM(condensed_music,"CONDENSED MUSIC","tools/condensed_music.xm")
// COCAINE COLA: the rework (tools/make_cocaine_cola_rework.py) of the stem transcription tools/cocaine_cola.xm, which stays as the secret original
SONG_XM(cocaine_cola_ii,"COCAINE COLA","tools/cocaine_cola_ii.xm")
SONG_XM(cocaine_cola,"COCAINE COLA (ORIGINAL)","tools/cocaine_cola.xm")
// STAGED THE FULL PERFORMANCE: a DayBar hi-NRG rework of The Dipper Man - Staged the Full Performance (tools/make_staged_rework.py, from tools/the_dipper_man_staged.xm)
SONG_XM(staged,"STAGED THE FULL PERFORMANCE","tools/staged.xm")

// CLOSER TO THE END: a faster, harder Danny Steele prog rework of The Dipper Man - Closer to the End (tools/make_closer_rework.py, 140 BPM, from tools/the_dipper_man_closer_to_the_end.xm)
SONG_XM(closer_to_the_end,"CLOSER TO THE END","tools/closer_to_the_end.xm")
// CLOSER TO THE END (ORIGINAL): the first, slower version (tools/make_closer_old_rework.py): a SECRET song, hidden until the title-screen code (isDbgSong in main.c)
SONG_XM(closer_to_the_end_old,"CLOSER TO THE END (ORIGINAL)","tools/closer_to_the_end_old.xm")
// THE TICKING BOMB: a VanInBlack Latin house rework of The Dipper Man - The Ticking Bomb (tools/make_tickingbomb_rework.py, 124 BPM with a half-time downgroove, from tools/the_dipper_man_ticking_bomb.xm)
SONG_XM(the_ticking_bomb,"THE TICKING BOMB","tools/the_ticking_bomb.xm")
// YOU'RE WINNER (ORIGINAL): Dipper - You're Winner (You Rule), transcribed from the song's stems (tools/make_winner.py, notes in
// tools/youre_winner_notes.json, 131 BPM, 6:07). A SECRET song, hidden until the title-screen code (isDbgSong in main.c), until it is approved.
SONG_XM(youre_winner,"YOU'RE WINNER (ORIGINAL)","tools/youre_winner.xm")
// HERE TODAY (MISSY'S SONG): the author's own MIDI (tools/here_today.mid) rendered by tools/make_heretoday_midi.py; the scene-5 lyric beats are timed to its phrases
SONG_ADP(here_today,"HERE TODAY (MISSY'S SONG)","source/music/here_today.adp")
