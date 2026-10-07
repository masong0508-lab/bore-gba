// pie.h - the PIE MENU (Sims style) for contextual interaction. Included by main.c right after menu().
// IWRAM SAFE: ROM code only (no IWRAM_* attributes), no static variables (every array is a local on the EWRAM stack),
// the tables are const (ROM). Nothing in this file adds a byte to IWRAM .bss/.data/.iwram.
//
// pieMenu: up to 8 entries ring a hub. D-PAD picks by direction (two keys = a diagonal), L R step round the ring,
// A confirms (returns the entry), B or START closes (-1). pieCats: more than 8 entries go in two levels, like the Sims:
// FRIENDLY / FUN / ROMANTIC / MEAN / USE first, then the actions of the one you picked (B steps back up a level).
static const char* const pieCatNm[5]={"FRIENDLY","FUN","ROMANTIC","MEAN","USE"};
static const u16 pieCatCol[5]={RGB(8,24,10),RGB(28,22,5),RGB(28,10,17),RGB(26,6,6),RGB(9,17,29)};
static const u8 pieCx[8]={120,184,206,184,120,56,34,56}, pieCy[8]={26,44,80,116,134,116,80,44};   // chip centres, clockwise from the top
#define PIE_HX 120
#define PIE_HY 80
#define PIE_HR 18
static void pieDisc(int cx,int cy,int r,u16 c){ for(int dy=-r;dy<=r;dy++){ int hw=r; while(hw*hw+dy*dy>r*r) hw--; rect(cx-hw,cy+dy,hw*2+1,1,c); } }
static void s2grad(int x,int y,int w,int h,int r0,int g0,int b0,int r1,int g1,int b1);   // (main.c, defined further down)
// The backdrop of every pie frame. fb does NOT hold the room (the room lives in VRAM and fb only keeps the last redraw patches, see "the life scene" in main.c),
// so drawing the pie over fb showed a stale, frozen mix of old patches, HUD text and the chips of the menu before. Every frame now paints the whole screen first.
static void pieBack(void){
    s2grad(0,0,SW,SH,2,6,14,1,3,8);
    for(int y=16;y<145;y+=8) for(int x=(y&8)?4:0;x<SW;x+=8) rect(x,y,1,1,RGB(4,9,16));   // a faint dot grid, like the story cards
}
static int pieDir(u16 k){   // the direction held: 0 up, 1 up-right, 2 right ... 7 up-left (-1: none)
    int u=(k&K_UP)!=0, d=(k&K_DOWN)!=0, l=(k&K_LEFT)!=0, r=(k&K_RIGHT)!=0;
    if(u&&r) return 1;
    if(d&&r) return 3;
    if(d&&l) return 5;
    if(u&&l) return 7;
    if(u) return 0;
    if(r) return 2;
    if(d) return 4;
    if(l) return 6;
    return -1;
}
static int pieMenu(const char*title,const char*foot,const char*const*lab,const u16*col,int n,int sel){   // n 1..8
    int slot[8], dirty=1; for(int i=0;i<n;i++) slot[i]=(i*8)/n;
    if(sel<0||sel>=n) sel=0;
    u16 prev=keyNow();
    sfxPlay(SFX_POP); objHideAll();
    pieBack();
    for(int g=1;g<=4;g++){   // the hub opens (it only grows, so nothing needs wiping)
        rect(0,0,SW,15,RGB(5,12,24)); rect(0,15,SW,1,GOLD); text((SW-tw(title,1))/2,4,title,GOLD,1);
        pieDisc(PIE_HX,PIE_HY,PIE_HR*g/4+1,GOLD); pieDisc(PIE_HX,PIE_HY,PIE_HR*g/4-1>0?PIE_HR*g/4-1:0,RGB(4,6,12)); present();
    }
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k;
        if(pr&(K_UP|K_DOWN|K_LEFT|K_RIGHT)){ int d=pieDir(k), best=sel, bd=99;
            if(d>=0) for(int i=0;i<n;i++){ int df=slot[i]>d?slot[i]-d:d-slot[i]; if(df>4) df=8-df; if(df<bd){ bd=df; best=i; } }
            if(best!=sel){ sel=best; dirty=1; sfxPlay(SFX_TICK); } }
        if(pr&K_R){ sel=(sel+1)%n; dirty=1; sfxPlay(SFX_TICK); }
        if(pr&K_L){ sel=(sel+n-1)%n; dirty=1; sfxPlay(SFX_TICK); }
        if(pr&K_A){ sfxPlay(SFX_POP); return sel; }
        if(pr&(K_B|K_START)) return -1;
        if(!dirty){ vsync(); continue; }
        dirty=0;
        pieBack();   // the whole screen again: no chip, spoke or HUD text of an earlier frame can survive
        rect(0,0,SW,15,RGB(5,12,24)); rect(0,15,SW,1,GOLD); text((SW-tw(title,1))/2,4,title,GOLD,1);
        rect(0,145,SW,15,RGB(5,12,24)); rect(0,145,SW,1,RGB(10,16,30));
        if(foot) text(6,150,foot,GOLD,1);
        { const char*h="A OK  B BACK"; text(SW-6-tw(h,1),150,h,RGB(16,18,22),1); }
        for(int i=0;i<n;i++) if(i!=sel) line(PIE_HX,PIE_HY,pieCx[slot[i]],pieCy[slot[i]],RGB(10,15,26));   // spokes, the chosen one on top
        line(PIE_HX,PIE_HY,pieCx[slot[sel]],pieCy[slot[sel]],col[sel]);
        pieDisc(PIE_HX,PIE_HY,PIE_HR+1,GOLD); pieDisc(PIE_HX,PIE_HY,PIE_HR-1,RGB(4,6,12));
        pieDisc(PIE_HX,PIE_HY,8,col[sel]); pieDisc(PIE_HX,PIE_HY,4,WHITE);
        for(int i=0;i<n;i++){ int on=(i==sel), w=tw(lab[i],1)+10, x=pieCx[slot[i]]-w/2, y=pieCy[slot[i]]-6;
            if(x<2) x=2;
            if(x+w>SW-2) x=SW-2-w;
            u16 c=col[i], dim=(u16)((c>>1)&0x3DEF);
            rect(x-1,y-1,w+2,14,on?WHITE:dim); rect(x,y,w,12,on?c:(u16)((dim>>1)&0x3DEF));
            text(x+5,y+2,lab[i],on?WHITE:DIMC,1); }
        present();
    }
}
static int pieCats(const char*title,const char*const*lab,const int*cat,int n){   // entry numbers back, -1 closed
    u16 col[8]; const char* l8[8];
    if(n<=0) return -1;
    if(n<=8){ for(int i=0;i<n;i++) col[i]=pieCatCol[cat[i]]; return pieMenu(title,0,lab,col,n,0); }
    int cnt[5]={0,0,0,0,0}, cs=0; for(int i=0;i<n;i++) cnt[cat[i]]++;
    for(;;){
        int ck[5], m=0; for(int k=0;k<5;k++) if(cnt[k]){ ck[m]=k; l8[m]=pieCatNm[k]; col[m]=pieCatCol[k]; m++; }
        int c=pieMenu(title,0,l8,col,m,cs); if(c<0) return -1;
        cs=c; int k=ck[c], idx[8], q=0;
        for(int i=0;i<n;i++) if(cat[i]==k&&q<8) idx[q++]=i;
        if(q==1) return idx[0];
        for(int j=0;j<q;j++){ l8[j]=lab[idx[j]]; col[j]=pieCatCol[k]; }
        int r=pieMenu(title,pieCatNm[k],l8,col,q,0); if(r>=0) return idx[r];   // B: back up to the categories
    }
}
