// unlocks.h - SONGS THAT START LOCKED. A song listed here is left out of the jukebox, the menu music and the game music until its unlock bit is set.
//
//   UNLOCK("SONG NAME",bit)     SONG NAME = the name exactly as it is written in source/songs.h; bit = UL_... (a power of two, defined in sims.h)
//
// The bits are saved in SRAM next to the jukebox flags (jukebox.h), so an unlock is for good; ERASE EVERYTHING locks them again, and the title-screen code
// (UP UP DOWN DOWN LEFT LEFT RIGHT B A START) shows locked songs too. jbUnlock(bit) in main.c sets a bit; today the only caller is simLtwCheck() in sims.h:
// WORTHLESS CLOUDS unlocks when the player meets their LIFETIME WANT.
UNLOCK("WORTHLESS CLOUDS",UL_CLOUDS)
