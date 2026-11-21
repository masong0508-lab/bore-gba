// A single perspective-correct, world-anchored architectural grammar.
// Window height, lintels, doors and masonry seams use PHYSICAL world
// height, never "pixels below roof" (which swam when camera dollied).
// All features are rendered inside the EXISTING 3DS world raycaster.
// No sprite overlays, heap, textures, or extra world passes.
#ifndef GG_FILM_FACADE3D_H
#define GG_FILM_FACADE3D_H
enum { GG_FACADE_WALL,GG_FACADE_COURSE,GG_FACADE_BEAM,
       GG_FACADE_FRAME,GG_FACADE_GLASS,GG_FACADE_DOOR,
       GG_FACADE_TRIM,GG_FACADE_PORTAL };
static inline int ggFilmFacadeHeightAt(int y,int horizon,int eye,
                                        int sample,int ground){
    if(sample<1)sample=1;
    return eye+(horizon-y)*(sample+1)/12-ground;
}
static inline int ggFilmFacadeFeature(int build,int u,int height){
    if(height<0)return GG_FACADE_WALL;
    int wood=build==B_HUT||build==B_HOUSE||
             build==B_DOCK||build==B_FARM;
    int stone=build==B_HALL||build==B_TOWER||
              build==B_LIBRARY||build==B_TEMPLE;
    if(!wood&&!stone)return GG_FACADE_WALL;
    // Tall central door is recessed in actual building wall and
    // capped by an arch/lintel; walls still mask it at other heights.
    int nearCenter=u>=105&&u<=151;
    if(nearCenter&&height<=12){
        if(u<111||u>145||height>=10)return GG_FACADE_FRAME;
        return GG_FACADE_DOOR;
    }
    if(stone&&nearCenter&&height>=13&&height<=18)
        return (u>115&&u<141&&height<=15)?
                  GG_FACADE_PORTAL:GG_FACADE_TRIM;
    // Two symmetric windows. Height changes by architecture type,
    // but stays fixed in WORLD HEIGHT when camera changes.
    // The previous 52-unit wide, 12-height glowing openings turned
    // halls into enormous yellow slabs. Narrow lancets with real stone
    // surrounds keep their correct WORLD position without dominating.
    int window=(u>=45&&u<=82)||(u>=174&&u<=211);
    int low=wood?8:17,high=wood?13:25;
    if(window&&height>=low-1&&height<=high+1){
        int border=height<=low||height>=high||
                   u<=48||u>=208||
                   (u>=80&&u<=82)||(u>=174&&u<=176)||
                   (u>=64&&u<=66)||(u>=189&&u<=191);
        return border?GG_FACADE_FRAME:GG_FACADE_GLASS;
    }
    if(wood){
        if(u<12||u>243||(u>=119&&u<=133))
            return GG_FACADE_BEAM;
        if(height%7==0)return GG_FACADE_COURSE;
    }else{
        if((height%6)==0)return GG_FACADE_COURSE;
        if(((u+((height/6)&1)*64)%128)<3)
            return GG_FACADE_COURSE;
        if(u<9||u>247)return GG_FACADE_TRIM;
    }
    return GG_FACADE_WALL;
}
#endif
