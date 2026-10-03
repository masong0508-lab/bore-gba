// songs.h - THE JUKEBOX SONG LIST. Order here = the "IN ORDER" order. Up to 64 songs.
//
//   SONG_XM(id,"NAME","file")    a tracker song (.xm). id = any unique C name, file = the .xm in tools/. After adding one, run
//                                python3 tools/xm2gba.py   to rebuild source/musicdata.h (the title music, THE DIPPER MAN, is one too)
//   SONG_ADP(id,"NAME","file")   a streamed ADPCM song. id = any unique C name, NAME = capitals / digits / spaces only
//                                (the on-screen font has no punctuation), file = a .adp made by tools/encode_song.py
//
// Easiest way to add a song:   python3 tools/encode_song.py "my song.mp3"
// It writes source/music/<id>.adp and appends the SONG_ADP line below for you.
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
SONG_XM(tree_swaying_action,"TREE SWAYING ACTION","tools/tree_swaying_action.xm")
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
// CYNICALLER MADNESS: a 172 BPM liquid / tech drum & bass rework of "The Dipper Man - Cynicaller Madness" on 64th-note rows
// (tools/make_cynicaller_rework.py, about 4:05)
SONG_XM(cynicaller_dnb,"CYNICALLER MADNESS","tools/cynicaller_dnb.xm")
