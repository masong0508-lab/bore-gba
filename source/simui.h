// simui.h - THE SIMS LOOK for the whole UI kit: The Sims 3's glossy rounded panels and pill buttons (s3*), The Sims 2's deep blue
// frames, gradients and green plumbob (s2*), a world that dims behind a dialog, the bubbles of the pie menu and the notification pill.
// box, menu, helpScreen and toast (main.c) are drawn with these, and so are house.h's pie menu, family.h's FAMILY panel and the
// birth / birthday notices. Needs before it: rect, text, tw, present, keyNow, objHideAll and fb (main.c).
static u16 s3Mix(u16 a,u16 b,int t,int n){ if(n<=1) return a; int m=n-1;
    int r=(a&31)+((int)(b&31)-(int)(a&31))*t/m, g=((a>>5)&31)+((int)((b>>5)&31)-(int)((a>>5)&31))*t/m, c=((a>>10)&31)+((int)((b>>10)&31)-(int)((a>>10)&31))*t/m; return RGB(r,g,c); }
static int s3Sq(int v){ int k=0; while((k+1)*(k+1)<=v) k++; return k; }
static int s3In(int j,int h,int r){ int e=j<r?2*(r-j)-1:j>=h-r?2*(j-(h-r))+1:0; return e?r-s3Sq(4*r*r-e*e)/2:0; }   // how far row j of a rounded box is pulled in
static void s3Box(int x,int y,int w,int h,int r,u16 top,u16 bot){ if(2*r>h) r=h/2; for(int j=0;j<h;j++){ int in=s3In(j,h,r); rect(x+in,y+j,w-2*in,1,s3Mix(top,bot,j,h)); } }
static void s3Panel(int x,int y,int w,int h){ s3Box(x,y,w,h,12,RGB(3,8,19),RGB(2,5,13)); s3Box(x+2,y+2,w-4,h-4,10,RGB(23,29,31),RGB(13,22,31)); rect(x+12,y+3,w-24,1,RGB(29,31,31)); }
static void s3Well(int x,int y,int w,int h){ s3Box(x,y,w,h,5,RGB(13,21,30),RGB(16,24,31)); s3Box(x+1,y+1,w-2,h-2,4,RGB(25,30,31),RGB(21,28,31)); }   // a sunken well (the town card, the tiles)
static void s3Pill(int x,int y,int w,int h,int on,const char*s){   // the focused one is green, like the game's
    s3Box(x,y,w,h,h/2,on?RGB(4,10,2):RGB(6,11,20),on?RGB(3,8,1):RGB(5,9,17));
    s3Box(x+1,y+1,w-2,h-2,(h-2)/2,on?RGB(21,30,9):RGB(29,31,31),on?RGB(10,22,2):RGB(18,25,31));
    s3Box(x+h/2,y+2,w-h,(h-4)/2,2,on?RGB(25,31,15):RGB(31,31,31),on?RGB(21,30,9):RGB(27,30,31));   // the gloss
    text(x+(w-tw(s,1))/2,y+(h-6)/2,s,on?RGB(1,4,0):RGB(2,5,11),1);
}
static void s3Tip(const char*t){ rect(0,150,SW,10,RGB(2,5,12)); rect(0,150,SW,1,RGB(8,14,26)); text((SW-tw(t,1))/2,152,t,RGB(26,29,31),1); }
static void s2rr(int x,int y,int w,int h,u16 c){ rect(x+1,y,w-2,h,c); rect(x,y+1,w,h-2,c); }   // a rounded rectangle
static void s2grad(int x,int y,int w,int h,int r0,int g0,int b0,int r1,int g1,int b1){        // a vertical gradient
    for(int i=0;i<h;i++){ int t=h>1?i*256/(h-1):0; rect(x,y+i,w,1,RGB(r0+(r1-r0)*t/256,g0+(g1-g0)*t/256,b0+(b1-b0)*t/256)); } }
static void s2plumbob(int cx,int y){   // the green diamond: dark left half, light right half, a glint
    for(int i=0;i<7;i++){ int hw=i<4?i:6-i; rect(cx-hw,y+i*2,hw+1,2,i<3?RGB(3,20,6):RGB(2,14,4)); rect(cx+1,y+i*2,hw,2,i<3?RGB(14,31,16):RGB(8,26,10)); }
    rect(cx+1,y+3,1,2,RGB(26,31,26)); }
static void s2pill(int x,int y,int w,const char*s){ s2rr(x,y,w,11,RGB(10,20,30)); s2grad(x+1,y+1,w-2,9,6,15,25,3,9,17); text(x+(w-tw(s,1))/2,y+2,s,RGB(20,27,31),1); }

// ---- the plumbob: green when a Sim feels fine, yellow when so-so, red when miserable (big: 14 rows, small: 10) ----
static const u16 suBobCol[3][4]={ {RGB(3,20,6),RGB(2,14,4),RGB(14,31,16),RGB(8,26,10)}, {RGB(22,18,2),RGB(16,12,1),RGB(31,29,10),RGB(28,24,4)},
    {RGB(22,4,3),RGB(15,2,2),RGB(31,14,12),RGB(28,8,6)} };
static void suBob(int cx,int y,int mood,int big){ const u16*c=suBobCol[mood<0?0:mood>2?2:mood]; int n=big?7:5, k=n/2;
    for(int i=0;i<n;i++){ int hw=i<=k?i:n-1-i; rect(cx-hw,y+i*2,hw+1,2,i<k?c[0]:c[1]); rect(cx+1,y+i*2,hw,2,i<k?c[2]:c[3]); }
    rect(cx+1,y+2,1,2,RGB(28,31,28)); }
static const signed char suBobY[8]={0,1,2,2,1,0,-1,-1};   // its bob, a step every 8 frames
// ---- behind a dialog the world dims: every pixel halfway to a deep navy (dimming again settles on the navy, never on black) ----
static void suDim(void){ u32*p=(u32*)fb; for(int i=0;i<SW*SH/2;i++) p[i]=((p[i]>>1)&0x3DEF3DEFu)+0x14411441u; }
// ---- the pie menu's backdrop: the world as a navy picture, kept at 4 bits a pixel past the mode 3 screen in VRAM (over the household
// sprite tiles, which are hidden in menus: hhSlotsFree has them uploaded again), so the menu can wipe what it drew ----
#define SU_BG ((volatile u32*)0x06012C00)
_Static_assert(0x06012C00+SW*SH/2<=0x06018000,"the pie backdrop must fit in VRAM");
static const u16 suNavy[16]={RGB(1,2,5),RGB(1,2,6),RGB(2,3,7),RGB(2,4,8),RGB(3,5,9),RGB(4,6,10),RGB(4,6,11),RGB(5,7,12),RGB(5,8,13),RGB(6,9,14),
    RGB(7,10,15),RGB(7,10,16),RGB(8,11,17),RGB(8,12,18),RGB(9,13,19),RGB(10,14,21)};
static void suBgLoad(void){ u32*o=(u32*)fb; for(int i=0;i<SW*SH/8;i++){ u32 w=SU_BG[i]; for(int j=0;j<4;j++){ *o++=(u32)suNavy[w&15]|((u32)suNavy[(w>>4)&15]<<16); w>>=8; } } }
static void suBgSave(void){   // fb becomes the navy picture (and is kept)
    for(int i=0;i<SW*SH/8;i++){ u32 w=0; const u16*q=&fb[i*8];
        for(int j=0;j<8;j++){ u16 c=q[j]; w|=(u32)((((c&31)*5+((c>>5)&31)*9+((c>>10)&31)*2)>>5)&15)<<(j*4); } SU_BG[i]=w; }
    suBgLoad(); }
static void suBgRect(int x,int y,int w,int h){   // wipe a rectangle back to the navy picture
    if(x<0){ w+=x; x=0; } if(y<0){ h+=y; y=0; } if(x+w>SW) w=SW-x; if(y+h>SH) h=SH-y;
    for(int j=0;j<h;j++){ int p=(y+j)*SW+x; for(int i=0;i<w;i++,p++) fb[p]=suNavy[(SU_BG[p>>3]>>((p&7)*4))&15]; } }
// ---- the pie menu's bubbles: Sims glass, tinted by what kind of thing it is; the one you point at turns green and glows ----
enum { SU_FRIEND, SU_ROM, SU_MEAN, SU_USE, SU_CAT, SU_TONES };
static const u16 suTone[SU_TONES][4]={   // rim, glass top, glass bottom, text
    {RGB(6,11,20),RGB(29,31,31),RGB(18,25,31),RGB(2,5,11)},     // friendly: the Sims' white-blue glass
    {RGB(15,4,10),RGB(31,27,30),RGB(29,16,23),RGB(11,1,6)},     // romance: pink
    {RGB(15,3,2),RGB(31,25,23),RGB(29,13,11),RGB(11,1,0)},      // mean: red
    {RGB(14,9,1),RGB(31,30,21),RGB(29,23,8),RGB(9,5,0)},        // use the furniture: gold
    {RGB(3,7,15),RGB(21,27,31),RGB(9,17,29),RGB(1,3,9)} };      // a group of them (FRIENDLY...): deeper blue
static int suBubbleW(const char*s){ return tw(s,1)+14; }
static void suBubble(int cx,int cy,const char*s,int tone,int on){
    int w=suBubbleW(s), h=13, x=cx-w/2, y=cy-h/2; if(x<2) x=2; if(x+w>SW-2) x=SW-2-w;
    const u16*t=suTone[tone];
    if(on) s3Box(x-2,y-2,w+4,h+4,(h+4)/2,RGB(28,31,18),RGB(16,29,8));   // the glow
    s3Box(x,y,w,h,h/2,on?RGB(4,10,2):t[0],on?RGB(3,8,1):t[0]);
    s3Box(x+1,y+1,w-2,h-2,(h-2)/2,on?RGB(21,30,9):t[1],on?RGB(10,22,2):t[2]);
    s3Box(x+h/2,y+2,w-h,(h-4)/2,2,RGB(31,31,31),on?RGB(21,30,9):t[1]);   // the gloss
    text(x+(w-tw(s,1))/2,y+(h-6)/2,s,on?RGB(1,4,0):t[3],1);
}
// ---- a relationship bar, Sims 2 style: a dark well with a centre notch, green to the right, red to the left ----
static void suRelBar(int x,int y,int w,int v){
    s2rr(x,y,w,6,RGB(9,16,26)); rect(x+1,y+1,w-2,4,RGB(2,4,9)); int c=x+w/2; rect(c,y,1,6,RGB(16,22,28));
    int f=v*(w/2-1)/100; if(f>0){ rect(c+1,y+1,f,4,RGB(7,24,6)); rect(c+1,y+1,f,1,RGB(16,31,12)); } else if(f<0){ rect(c+f,y+1,-f,4,RGB(25,6,4)); rect(c+f,y+1,-f,1,RGB(31,15,12)); }
}
// ---- the title bar of a panel: a lit gradient, the bobbing plumbob, the title ----
static void suTitleBar(int x,int y,int w,const char*t,u32 frame){
    s2grad(x,y,w,13,9,19,29,3,10,20); rect(x,y+13,w,1,RGB(15,26,31)); rect(x+2,y,w-4,1,RGB(18,27,31));
    suBob(x+8,y+2+(suBobY[(frame>>3)&7]>>1),0,0); text(x+16,y+4,t,WHITE,1); }
// ---- a full-screen Sims 2 backdrop: deep blue with a faint diamond lattice, framed ----
static void suBackdrop(void){
    s2grad(0,0,SW,SH,1,4,10,2,9,17);
    for(int y=0;y<SH;y+=8) for(int x=(y&8)?4:0;x<SW;x+=8) rect(x,y,1,1,RGB(3,9,17));
    s2rr(1,1,238,158,RGB(10,20,30)); s2rr(2,2,236,156,RGB(2,6,13)); }
// ---- little badges: a gold ring with a stone (married), a heart (going steady) ----
static void suRing(int x,int y){ static const signed char cx[16]={0,1,2,3,3,3,2,1,0,-1,-2,-3,-3,-3,-2,-1}, cy[16]={-3,-3,-2,-1,0,1,2,3,3,3,2,1,0,-1,-2,-3};
    for(int a=0;a<16;a++) rect(x+cx[a],y+cy[a],1,1,RGB(31,27,8)); rect(x,y-4,1,1,RGB(25,31,31)); }
static void suHeart(int x,int y){ static const u8 hb[5]={0x36,0x7F,0x3E,0x1C,0x08}; for(int j=0;j<5;j++) for(int i=0;i<7;i++) if(hb[j]>>(6-i)&1) rect(x-3+i,y-2+j,1,1,RGB(31,10,18)); }
