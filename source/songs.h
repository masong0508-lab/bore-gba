// songs.h - THE JUKEBOX SONG LIST. Order here = the "IN ORDER" order. Up to 32 songs.
//
//   SONG_XM("NAME")              the built-in tracker song (the title music, tools/the_dipper_man.xm)
//   SONG_ADP(id,"NAME","file")   a streamed ADPCM song. id = any unique C name, NAME = capitals / digits / spaces only
//                                (the on-screen font has no punctuation), file = a .adp made by tools/encode_song.py
//
// Easiest way to add a song:   python3 tools/encode_song.py "my song.mp3"
// It writes source/music/<id>.adp and appends the SONG_ADP line below for you.
//
// Song spec: 4-bit IMA-ADPCM, mono, 18157 Hz (the game's music rate), about 9 KB per second of music.
// When this list changes size the saved shuffle is re-rolled automatically (next boot).

SONG_XM("THE DIPPER MAN")

// ---- PLACEHOLDERS (short test tunes so shuffle / select can be heard now) ----
// Delete these 3 lines and the 3 files in source/music/ when you add your real songs.
SONG_ADP(placeholder_a,"PLACEHOLDER A","source/music/placeholder_a.adp")
SONG_ADP(placeholder_b,"PLACEHOLDER B","source/music/placeholder_b.adp")
SONG_ADP(placeholder_c,"PLACEHOLDER C","source/music/placeholder_c.adp")

// ---- songs added by tools/encode_song.py go below this line ----
SONG_ADP(earth_and_the_space_citizens,"EARTH AND THE SPACE CITIZENS","source/music/earth_and_the_space_citizens.adp")
