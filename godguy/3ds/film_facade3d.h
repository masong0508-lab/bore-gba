// A single perspective-correct, world-anchored architectural grammar.
// Window height, lintels, doors and masonry seams use PHYSICAL world
// height, never "pixels below roof" (which swam when camera dollied).
// All features are rendered inside the EXISTING 3DS world raycaster.
// No sprite overlays, heap, textures, or extra world passes.
#ifndef GG_FILM_FACADE3D_H
#define GG_FILM_FACADE3D_H
enum { GG_FACADE_WALL,GG_FACADE_COURSE,GG_FACADE_BEAM,
       GG_FACADE_FRAME,GG_FACADE_GLASS,GG_FACADE_DOOR,
       GG_FACADE_TRIM,GG_FACADE_PORTAL,GG_FACADE_SILL,
       GG_FACADE_CORNICE,GG_FACADE_BRACE,GG_FACADE_MOSS,
       GG_FACADE_SCULPT };
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
              build==B_LIBRARY||build==B_TEMPLE||
              build==B_RUIN||build==B_STATUE;
    if(!wood&&!stone)return GG_FACADE_WALL;
    // Lesser structures need their own physical material story:
    // dock pilings, agricultural frames, ruin fragmentation and
    // the statue pedestal should never be plain blank skyscrapers.
    if(build==B_FARM)
        return height<=5&&((u&31)<4||((u>>4)&1)&&height==3)?
               GG_FACADE_BRACE:GG_FACADE_WALL;
    if(build==B_DOCK)
        return height<=6&&(u<16||u>238||((u>>6)&1)&&height==5)?
               GG_FACADE_BRACE:GG_FACADE_WALL;
    if(build==B_RUIN){
        if(height>=9&&((u*5+height*7)&31)<13)
            return GG_FACADE_MOSS;
        if(height%4==0)return GG_FACADE_COURSE;
        if(u>=92&&u<=166&&height<=8)
            return GG_FACADE_PORTAL;
        return GG_FACADE_WALL;
    }
    if(build==B_STATUE){
        if(height<5)return GG_FACADE_CORNICE;
        return (u>=72&&u<=184&&height<=21)?
                  GG_FACADE_SCULPT:GG_FACADE_WALL;
    }
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
    if(window&&height==low-2)return GG_FACADE_SILL;
    if(window&&height>=low-1&&height<=high+1){
        int border=height<=low||height>=high||
                   u<=48||u>=208||
                   (u>=80&&u<=82)||(u>=174&&u<=176)||
                   (u>=64&&u<=66)||(u>=189&&u<=191);
        return border?GG_FACADE_FRAME:GG_FACADE_GLASS;
    }
    if((stone&&height==29)||(wood&&height==15&&u<30))
        return GG_FACADE_CORNICE;
    if(wood){
        if((height==6||height==13)&&
           ((u>19&&u<44)||(u>211&&u<236)))
            return GG_FACADE_BRACE;
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
