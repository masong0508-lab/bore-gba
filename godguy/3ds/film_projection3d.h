// One cinematic perspective contract shared by the raycast scene, camera
// scorer and live 2.5D characters. Q8 distances, integer-only projection.
// This prevents the old hero/guest billboard scaling and hover mismatch.
#ifndef GG_FILM_PROJECTION3D_H
#define GG_FILM_PROJECTION3D_H
enum { GG_FILM_HORIZON=116, GG_FILM_FOCAL=250,
       GG_FILM_EYE_HEIGHT=13, GG_FILM_HEIGHT_MULT=12 };
// Use the EXACT same horizontal focal length as ggExpRenderMode's
// screen ray (skew / 250), rather than the former incompatible 140.
static int ggFilmProjectScreenX(int side,int forward){
    if(forward<=0)return -10000;
    return 200+side*GG_FILM_FOCAL/forward;
}
// The renderer draws visible land at horizon-(height-eye)*12/(d+1).
// A character's boots must be at the same WORLD GROUND projection, not
// the horizon. The old calculation omitted the camera's 13-unit eye
// height and left every actor hovering dozens of pixels above grass.
static int ggFilmProjectFootY(int forward,int cameraGround,int actorGround){
    if(forward<=0)return -10000;
    // At whole-tile distances this exactly matches the raycaster's
    // d+1 denominator. Interpolate in Q8 between tile samples, so a
    // tracking dolly does not pop boots up or down when forward crosses
    // the next 256-unit boundary.
    return GG_FILM_HORIZON-(actorGround-cameraGround-
           GG_FILM_EYE_HEIGHT)*GG_FILM_HEIGHT_MULT*256/
           (forward+512);
}
// Scaling is an authored camera-take property, NOT rounded independently
// every time the actor moves a fraction of a tile. Hero and guests share
// one aspect ratio and do not pop between 1x/2x/3x across a dolly.
static int ggFilmShotScale(int cameraDistanceTiles){
    if(cameraDistanceTiles<=2)return 3;
    if(cameraDistanceTiles<=5)return 2;
    return 1;
}
// All 32px character bodies must remain between the chapter header and
// the caption band. Return false for a bad shot rather than render a
// chopped-off giant or a leg peeking through the subtitles.
static int ggFilmFootSafe(int screenX,int feet,int scale){
    int half=10*scale;
    return screenX>=half+6&&screenX<400-half-6&&
           feet>=34+32*scale&&feet<=184;
}
#endif
