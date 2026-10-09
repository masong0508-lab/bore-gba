// tvclip.h - the TV's 3 second clips. A clip is drawn by code (no video data in the ROM): a tick-driven loop, one picture per frame, that
// takes over the screen like a menu does and hands it back when it ends. A or B skips it. Called from lifeModeRun (main.c) when homeUse
// (skills.h) asked for one with tvClip = channel + 1. Needs before it: rect, text, tw, present, keyNow, sfxPlay, clipAll, RGB.
//
// Channel 0, MACHINI-TV: a slowly scrolling rainbow, a bouncing smiley, big words that pop up every third of a second, a like
// counter that runs away and a ticker. Easy on the eyes on purpose: the colours drift, nothing flashes the whole screen.
#define TV_FRAMES 180                // 3 seconds at 60 frames a second
#define TV_BLK RGB(1,1,2)

static u16 tvHue(int h,int dim){     // a rainbow colour (h = 0..191 round the wheel), dimmed by `dim` (0 = full, 1 = half)
    h%=192; if(h<0) h+=192; int s=h>>5, f=h&31, r, g, b;
    switch(s){ case 0: r=31; g=f; b=0; break; case 1: r=31-f; g=31; b=0; break; case 2: r=0; g=31; b=f; break;
               case 3: r=0; g=31-f; b=31; break; case 4: r=f; g=0; b=31; break; default: r=31; g=0; b=31-f; }
    return RGB(r>>dim,g>>dim,b>>dim);
}
static int tvTri(int t,int span){    // a triangle wave 0..span..0 (for the bouncing)
    int p=t%(2*span); return p<span?p:2*span-p;
}
static char* tvNum(char*d,int v){    // v as digits at d, returns the end
    char t[12]; int n=0; if(v<=0) t[n++]='0'; while(v>0){ t[n++]=(char)('0'+v%10); v/=10; }
    while(n>0){ *d++=t[--n]; }
    *d=0; return d;
}
static void tvClipRun(int ch){
    (void)ch;   // channel 0 is the only clip so far; more channels get their own picture here
    static const char* const word[8]={"MACHINI","ON AIR","LIVE","TUNE IN","STAY TUNED","WOW","NEW","MACHINI-TV"};
    static const signed char shk[6]={0,3,-3,2,-1,0};
    static const char tick[]="MACHINI-TV   STAY TUNED   NEW CLIP EVERY 3 SECONDS   ";
    clipAll();
    u16 prev=keyNow();
    int tickW=tw(tick,1);
    for(int f=0;f<TV_FRAMES;f++){
        u16 k=keyNow(), pr=k&~prev; prev=k; if(pr&(K_A|K_B|K_START)) break;
        for(int y=0;y<SH;y+=8) rect(0,y,SW,8,tvHue(y*2+f*2,1));                    // the rainbow, scrolling
        // the bouncing smiley
        int sx=14+tvTri(f*3,176), sy=22+tvTri(f*2,84);
        rect(sx+4,sy,16,24,RGB(31,27,3)); rect(sx,sy+4,24,16,RGB(31,27,3)); rect(sx+2,sy+2,20,20,RGB(31,27,3));
        rect(sx+7,sy+7,3,5,TV_BLK); rect(sx+14,sy+7,3,5,TV_BLK);
        rect(sx+6,sy+16,12,2,TV_BLK); rect(sx+5,sy+14,2,2,TV_BLK); rect(sx+17,sy+14,2,2,TV_BLK);
        // the big word, a new one every 22 frames, with a little shake as it lands
        int wi=(f/22)&7, t=f%22; const char*w=word[wi];
        int sc=(tw(w,3)<=224)?3:2, ww=tw(w,sc), wx=(SW-ww)/2+(t<6?shk[t]:0), wy=62+(t<6?shk[5-t]:0);
        text(wx+2,wy+2,w,TV_BLK,sc); text(wx,wy,w,RGB(31,31,31),sc);
        if(t==0) sfxPlay(SFX_POP);
        // the bezel: a title bar on top, the like counter and the ticker below
        rect(0,0,SW,12,TV_BLK); text(6,3,"CH 1  MACHINI-TV",RGB(31,31,31),1);
        { char b[16]; char*e=tvNum(b,1000+f*f*53); (void)e; int x=SW-6-tw(b,1); text(x,3,b,RGB(31,12,12),1); text(x-tw("LIKES ",1),3,"LIKES ",RGB(31,31,31),1); }
        rect(0,146,SW,14,TV_BLK);
        { int x=SW-((f*3)%(SW+tickW)); text(x,150,tick,RGB(31,31,6),1); }
        rect(0,0,4,SH,TV_BLK); rect(SW-4,0,4,SH,TV_BLK);
        present();
    }
    voxPlay(V_laughing);
    rect(0,0,SW,SH,TV_BLK); present();                                            // the set switches off
}
