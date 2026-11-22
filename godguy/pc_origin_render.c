// Compile the actual New 3DS origin renderer without requiring libctru.
#define GG_TEST_NATIVE_ORIGIN
#include "3ds/main3ds.c"

static struct { u8 before[16],rgb[400*240*3],after[16]; } frame;
static u32 sceneHash(void){
    u32 h=2166136261u;
    for(int i=40*400*3;i<175*400*3;i++)h=(h^frame.rgb[i])*16777619u;
    return h;
}
static void check(int ok,const char *message){
    if(!ok){fprintf(stderr,"ORIGIN RENDER FAIL: %s\n",message);exit(1);}
}
int main(void){
    static const int phases[]={GG_ORIGIN_BIRTH,GG_ORIGIN_CHILD,GG_ORIGIN_YOUTH,GG_ORIGIN_AWAKEN};
    ggOrigin.active=1;
    memset(uiBuf,0x7a,sizeof uiBuf);
    memset(frame.before,0x5a,sizeof frame.before);
    memset(frame.after,0x6b,sizeof frame.after);
    for(unsigned i=0;i<sizeof phases/sizeof *phases;i++){
        ggOrigin.phase=phases[i];
        ggOrigin.frame=0;
        ggOriginDraw(frame.rgb);
        u32 start=sceneHash();
        ggOrigin.frame=53;
        ggOriginDraw(frame.rgb);
        check(start!=sceneHash(),"a projected scene freezes as the camera moves");
        for(unsigned j=0;j<sizeof uiBuf;j++)
            check(uiBuf[j]==0x7a,"drawing the origin alters the shared game UI");
        for(int j=0;j<16;j++)
            check(frame.before[j]==0x5a&&frame.after[j]==0x6b,
                  "the origin writes past the RGB framebuffer");
    }
    puts("ORIGIN RENDER OK: four moving projected scenes, separate UI and bounded RGB output");
    return 0;
}
