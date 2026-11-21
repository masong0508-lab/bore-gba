// GodGuy 3DS film: deterministic secondary acting, no random animation.
// These motion phases are a pure function of film-time, so skipping,
// rewinding or pausing a scene preserves continuity. They NEVER move the
// world-space foot anchor, terrain, stage position or camera.
#ifndef GG_FILM_LIFE3D_H
#define GG_FILM_LIFE3D_H
static inline int ggFilmLifePulse(int frame,int phase){
    int t=(frame+phase)%72;
    if(t<0)t+=72;
    // One pixel of quiet torso lift through a slow breath, with a
    // deliberate hold. All cast remain exactly foot-planted.
    return t>=19&&t<45?1:0;
}
static inline int ggFilmLifeWeight(int frame,int phase){
    int t=(frame+phase)%112;
    if(t<0)t+=112;
    return t<22?-1:t<49?0:t<78?1:0;
}
static inline int ggFilmLifeListenCue(int local,int beat){
    if(local<19||local>=85)return 0;
    if((beat&1)==0)return local<34?1:local<45?-1:0;
    return local>=53&&local<69?-1:0;
}
static inline int ggFilmLifeBrow(int speaking,int pose,int local){
    if(pose==3||pose==5)return 1; // presenting/comforting
    if(pose==4)return -1;          // worried or reflective
    return speaking&&local>35&&local<60?1:0;
}
#endif
