#ifndef GG_STORY_WORLD3D_PALETTE_H
#define GG_STORY_WORLD3D_PALETTE_H

// Dedicated, unused sub-palette slots between the UI, minimap and title ranges.
// The original 32 film colors stay available for cels and props.
enum {
    GG_SKY_HIGH=32, GG_SKY_MIST, GG_CLOUD, GG_SKY_RAIN,
    GG_WATER_DARK, GG_WATER_CREST, GG_WATER_GLINT,
    GG_PINE, GG_LEAF, GG_MOSS,
    GG_SLATE, GG_STONE, GG_LIMESTONE, GG_PLASTER,
    GG_COPPER, GG_OXBLOOD, GG_FILM_INK_N
};
enum { GG_FILM_MATERIALS=GG_FILM_INK_N-32 };
static const u8 ggFilmMaterialSlots[GG_FILM_MATERIALS]={
    31,77,78,79,86,87,88,89,90,91,92,93,94,95,254,255,
};
#define GG_RGB15(r,g,b) ((r)|((g)<<5)|((b)<<10))
static const unsigned short ggFilmMaterialPalette[GG_FILM_MATERIALS]={
    GG_RGB15(13,21,27), GG_RGB15(21,25,26), GG_RGB15(26,27,27), GG_RGB15(8,13,18),
    GG_RGB15(3,9,14), GG_RGB15(9,20,21), GG_RGB15(18,25,24), GG_RGB15(5,12,12),
    GG_RGB15(9,18,13), GG_RGB15(15,20,14), GG_RGB15(9,12,17), GG_RGB15(16,18,19),
    GG_RGB15(21,21,18), GG_RGB15(25,20,14), GG_RGB15(21,11,8), GG_RGB15(11,6,9),
};
#undef GG_RGB15
static inline u8 ggFilmInk(u8 ink){
    return ink<32?fmvSlots[ink]:ggFilmMaterialSlots[ink-32];
}
#endif
