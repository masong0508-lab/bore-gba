#ifndef GG_STORY_WORLD3D_MATERIAL_H
#define GG_STORY_WORLD3D_MATERIAL_H

static void ggFilmFace(GgFilmCamera cam,GgV3 a,GgV3 b,GgV3 c,u8 ink){
    ggFilmTri(cam.page,ggFilmProject(cam,a),ggFilmProject(cam,b),
              ggFilmProject(cam,c),ink);
}
static void ggFilmPanel(GgFilmCamera cam,int l,int r,int b,int t,int z,u8 ink){
    ggFilmQuad(cam,ggV3(l,b,z),ggV3(r,b,z),ggV3(r,t,z),ggV3(l,t,z),ink);
}
static void ggFilmPaving(GgFilmCamera cam,int l,int r,int n,int f,int y,u8 ink){
    ggFilmQuad(cam,ggV3(l,y,n),ggV3(r,y,n),ggV3(r,y,f),ggV3(l,y,f),ink);
}
// Tapered cuts in an exposed surface, used for wood grain and stone seams.
static void ggFilmGrain(GgFilmCamera cam,int l,int r,int y,int z,int h,u8 ink){
    ggFilmFace(cam,ggV3(l,y,z),ggV3(r,y,z),ggV3(l+(r-l)/3,y+h,z),ink);
}
// A bevel with an irregular plan, an asymmetric top and three exposed faces.
// Palette order follows light from the upper left of the set.
static void ggFilmRock(GgFilmCamera cam,int x,int y,int z,int w,int h,int d,
                       u8 lit,u8 face,u8 shade){
    GgV3 front=ggV3(x+w/7,y,z-d/2);
    GgV3 left=ggV3(x-w/2,y,z-d/4);
    GgV3 right=ggV3(x+w/2,y,z+d/8);
    GgV3 rear=ggV3(x-w/6,y,z+d/2);
    GgV3 a=ggV3(x+w/8,y+h/2,z-d/3);
    GgV3 b=ggV3(x-w/3,y+h*3/4,z-d/6);
    GgV3 c=ggV3(x+w/3,y+h/2,z+d/10);
    GgV3 d0=ggV3(x-w/8,y+h,z+d/5);
    ggFilmQuad(cam,left,rear,d0,b,face);
    ggFilmQuad(cam,rear,right,c,d0,shade);
    ggFilmQuad(cam,left,front,a,b,face);
    ggFilmQuad(cam,front,right,c,a,shade);
    ggFilmQuad(cam,b,a,c,d0,lit);
}
// Contact shadows and gravel skirts are part of the surface plane.
static void ggFilmGroundPatch(GgFilmCamera cam,int x,int y,int z,int w,int d,u8 ink){
    GgV3 a=ggV3(x-w/2,y,z-d/4),b=ggV3(x+w/3,y,z-d/2);
    GgV3 c=ggV3(x+w/2,y,z+d/4),e=ggV3(x-w/3,y,z+d/2);
    ggFilmQuad(cam,a,b,c,e,ink);
}
// Three bent blades share a root. The empty gaps retain the ground's color.
static void ggFilmTuft(GgFilmCamera cam,int x,int y,int z,int h,int lean,u8 ink){
    ggFilmFace(cam,ggV3(x-6,y,z),ggV3(x+4,y,z+3),ggV3(x-13+lean,y+h,z+4),ink);
    ggFilmFace(cam,ggV3(x,y,z-2),ggV3(x+7,y,z),ggV3(x+3+lean,y+h+h/3,z+6),ink);
    ggFilmFace(cam,ggV3(x+5,y,z+3),ggV3(x+12,y,z+5),ggV3(x+18+lean,y+h*2/3,z+8),ink);
}
// Stone courses are face-local tessellation. Wider faces get running bond
// and discontinuous chips while the light side keeps its large clean planes.
static void ggFilmMasonry(GgFilmCamera cam,int l,int r,int bottom,int top,int z,
                          int rise,int width,u8 mortar,u8 light,u8 mid){
    ggFilmPanel(cam,l,r,bottom,top,z,mortar);
    for(int y=bottom,row=0;y<top;y+=rise,row++){
        int cap=y+rise-3;
        if(cap>top)cap=top;
        for(int x=l-(row&1)*width/2,i=0;x<r;x+=width,i++){
            int a=x<l?l:x,b=x+width-4;
            if(b>r)b=r;
            if(a>=b)continue;
            u8 stone=(i+row*2)%4==0?light:mid;
            ggFilmPanel(cam,a,b,y,cap,z-1,stone);
        }
    }
}
#endif
