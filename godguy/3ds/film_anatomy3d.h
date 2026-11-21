// Fixed, character-specific 3DS film anatomy, 63 bytes of authored
// measurements (not skinned meshes). One shape per character identity.
// The same world-space foot anchor and 1x/2x/3x shot scale are retained.
#ifndef GG_FILM_ANATOMY3D_H
#define GG_FILM_ANATOMY3D_H
typedef struct {
    signed char shoulder,hip,skull,cheek,jaw,faceHeight;
    signed char nose,eyeGap,shoulderHalf;
} GgFilmAnatomy;
static inline GgFilmAnatomy ggFilmAnatomyFor(int style){
    static const GgFilmAnatomy identity[7]={
        {12,11,10,8,6,9,1,2,6},   // Mira: lithe, tapered jaw
        {18,15,12,10,10,9,2,3,8}, // Thorgrim: powerful, square jaw
        {15,12,11,8,7,9,2,3,7},   // Varro: high martial shoulders
        {11,10,9,7,5,11,3,2,5},   // Serin: narrow, long sharp profile
        {13,13,10,9,6,9,1,3,6},   // Elowen: high mantle, fine chin
        {14,12,10,8,7,9,1,2,6},   // Citizen
        {13,12,10,8,7,9,1,2,6}    // Player-customized figure
    };
    if(style<0||style>6)style=6;
    return identity[style];
}
static inline int ggFilmAnatomyWidth(int width,int turn,int minimum){
    int t=turn<0?-turn:turn;
    if(t>4)t=4;
    int result=width-t;
    return result<minimum?minimum:result;
}
#endif
