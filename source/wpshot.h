// wpshot.h - WEAPONS (module 2: shots). PISTOL and UZI bullets, and the ROCKET.
// AIM      the D-pad, 8 ways, at the moment you fire (wpAim, weapons.h); pad let go = the way you face. Works on the board too.
// BULLETS  fly straight and fast, stop at walls, hurt the first Sim they touch.
// MISSILE  slow, and GUIDED: keep R held after the shot and the D-pad steers it, 8 ways, every step. Let go of R and it flies straight on.
//          It blows up on a wall or a Sim (or when its fuel runs out): everyone within 1.6 tiles is hurt (less the further out), and so are you if you stand close.
// SIZE     6 shots at a time, 16 bytes each, and one blast. Nothing is saved.
#define WP_SHOTS 6
typedef struct { s32 x, y; u8 kind, life, w; s8 dx, dy; } WpShot;   // kind 0 free, 1 bullet, 2 missile
static WpShot wpS[WP_SHOTS] EWRAM_BSS; static u8 wpBoomT EWRAM_BSS; static s32 wpBoomX EWRAM_BSS, wpBoomY EWRAM_BSS;
static void wpShoot(int w,int ax,int ay){
    int i=0; while(i<WP_SHOTS&&wpS[i].kind) i++;
    if(i==WP_SHOTS){ lnote="TOO MANY SHOTS"; lnoteT=20; return; }
    WpShot*s=&wpS[i]; int d=(ax&&ay)?36:48;   // the muzzle: a little ahead of you
    s->x=lfx+ax*d; s->y=lfy+ay*d; s->dx=(s8)ax; s->dy=(s8)ay; s->w=(u8)w; s->kind=(u8)(wpT[w].kind); s->life=(u8)(wpT[w].kind==2?110:44);
    sfxPlay(wpT[w].kind==2?SFX_THUNDER:SFX_HIT);
}
static void wpBoom(s32 x,int dmg){   // (x is passed with wpBoomY set by the caller)
    wpBoomT=14; wpBoomX=x; sfxPlay(SFX_THUNDER);
    int R=410;
    for(int m=0;m<hhN;m++){ if(!wpLive(m)) continue;
        int dx=(int)(hhM[m].fx-wpBoomX), dy=(int)(hhM[m].fy-wpBoomY), q=dx*dx+dy*dy; if(q>=R*R) continue;
        int d=1; while(d*d<q) d++;   // (square root: the distances are small)
        wpHurt(m,10+(dmg-10)*(R-d)/R,WP_ROCKET); }
    { int dx=(int)(lfx-wpBoomX), dy=(int)(lfy-wpBoomY), q=dx*dx+dy*dy; if(q<330*330&&!ldead){ fightHurt(dmg/3); lnote="CAUGHT IN YOUR OWN BLAST"; lnoteT=50; } }
}
static void wpShotTick(void){
    if(wpBoomT) wpBoomT--;
    for(int i=0;i<WP_SHOTS;i++){ WpShot*s=&wpS[i]; if(!s->kind) continue;
        if(s->kind==2&&(wpKeys&K_R)&&(wpKeys&(K_UP|K_DOWN|K_LEFT|K_RIGHT))){ int ax,ay; wpAim(wpKeys,&ax,&ay); s->dx=(s8)ax; s->dy=(s8)ay; }   // GUIDED: R held, the D-pad steers
        int sp=s->kind==2?52:92; if(s->dx&&s->dy) sp=sp*3/4;
        s->x+=s->dx*sp; s->y+=s->dy*sp;
        int tx=(int)(s->x>>8), ty=(int)(s->y>>8);
        int end=0;   // 1 = vanished, 2 = blew up
        if(s->x<0||s->y<0||tx>=MW||ty>=MH) end=1;
        else if(tileH(tx,ty)>=14) end=2;   // a wall or tall furniture
        else if(--s->life==0) end=s->kind==2?2:1;
        if(!end) for(int m=0;m<hhN&&!end;m++){ if(!wpLive(m)) continue;
            int dx=(int)(hhM[m].fx-s->x), dy=(int)(hhM[m].fy-s->y);
            if(dx*dx+dy*dy<115*115){ if(s->kind==1) wpHurt(m,wpT[s->w].dmg+(rnd8()>>5),s->w); end=2; } }
        if(end){ if(s->kind==2){ wpBoomY=s->y; wpBoom(s->x,wpT[s->w].dmg); } s->kind=0; }
    }
}
static void wpShotDraw(void){
    for(int i=0;i<WP_SHOTS;i++){ const WpShot*s=&wpS[i]; if(!s->kind) continue; int sx,sy;
        wpScr(s->x,s->y,10,&sx,&sy);
        if(s->kind==1){ int tx,ty; wpScr(s->x-s->dx*70,s->y-s->dy*70,10,&tx,&ty); px((sx+tx)/2,(sy+ty)/2,RGB(31,24,8)); px(tx,ty,RGB(20,14,4)); rect(sx-1,sy-1,2,2,RGB(31,31,20)); }
        else { for(int t=1;t<=3;t++){ int tx,ty; wpScr(s->x-s->dx*40*t,s->y-s->dy*40*t,10+t,&tx,&ty); rect(tx,ty,2,2,t==1?RGB(31,18,4):RGB(18,18,18)); }   // the smoke
            rect(sx-2,sy-2,4,4,RGB(28,6,4)); rect(sx-1,sy-1,2,2,RGB(31,30,24)); } }
    if(wpBoomT){ int sx,sy; wpScr(wpBoomX,wpBoomY,6,&sx,&sy); int r=(14-wpBoomT)*2+3; if(r>22) r=22;
        for(int dy=-r/2;dy<=r/2;dy++){ int ww=0; while((ww+1)*(ww+1)+dy*dy*4<=r*r) ww++;   // a flattened circle: the floor is seen at an angle
            rect(sx-ww,sy+dy,2*ww+1,1,(wpBoomT>8)?RGB(31,28,12):(wpBoomT>4)?RGB(31,14,3):RGB(14,6,3)); } }
}
