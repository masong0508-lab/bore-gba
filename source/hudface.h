// hudface.h - the mood portrait: the controlled Sim's own face (creator eye and mouth sprites, face sliders applied). Included by hud.h.
// ---- the mood portrait: the controlled Sim's own face, drawn with the creator's eye and mouth sprites (spr[], 9x8 eyes and 19x8 mouths) at 1x ----
// Each mood picks an expression: an eye sprite, a mouth sprite, brows (0 = the Sim's own BROWS look, else brArt style n) and tears. The Sim's skin, iris, hair,
// brows, glasses, nose and cheeks come from `look`, so the face stays theirs. To change an expression, edit one row of hudExpr (eye ids 0..8, mouth ids 0..8 are
// the order of the EYES and MOUTH rows in the creator: SLEEPY ROUND HAPPY WIDE ANGRY CUTE CAT DOT LASHES / FLAT SMILE OH GRIN SMIRK FROWN TONGUE FANGS KITTY).
static const u8 hudExpr[5][4]={   // eye, mouth, brows, tears
    {5,5,4,1},   // SAD      big wet CUTE eyes, SAD brows, FROWN, a tear on each side
    {0,0,0,0},   // BORED    SLEEPY half-shut eyes, FLAT mouth
    {1,0,0,0},   // OK       ROUND eyes, FLAT mouth
    {2,1,0,0},   // HAPPY    HAPPY (closed, smiling) eyes, SMILE
    {3,3,0,0} }; // STOKED   WIDE eyes, GRIN
static const u8 hudHairRows[NHAIR]={2,3,3,0,3,4,2,3,3};   // rows of hair across the top of the portrait per hair style (CROP BOWL LONG BALD SPIKY AFRO FLAT TOP SIDE TAIL BUN)
static u16 hudHairCol(void){ return toneBy(hairTones[look[LK_HCOL]%NSW],slideEffS(look[LK_HTONE])); }
static void hudPx(int x,int y,u16 c){ rect(x,y,1,1,c); }
static void hudSprPlot(int x,int y,const char*a,int w,int fl,const u16*pal){   // one art row, 'k w r s i b g h l' as in setColors, 't' a tear; a is w chars wide
    static const char key[]="kwrsibghlt";
    for(int i=0;i<w;i++){ char c=a[fl?w-1-i:i]; if(c=='.') continue; int k=0; while(key[k]&&key[k]!=c) k++; if(key[k]) hudPx(x+i,y,pal[k]); }
}
static int hudFl(int a,int b){ return a>=0?a/b:-((-a+b-1)/b); }   // floor division
static void hudRng(int d,int c,int k,int*a,int*b){   // the art columns (or rows) that destination pixel d covers when the art is scaled by k/64 about c
    *a=c+hudFl((d-c)*64,k); *b=c+hudFl((d+1-c)*64,k)-1; if(*b<*a) *b=*a;
}
static char hudPick(const char*const*A,int rmax,int w,int fl,int a0,int a1,int r0,int r1){   // the first ink in the box, row by row (as drawDeco's PICK); '.' outside the art
    if(a0<0) a0=0;
    if(a1>w-1) a1=w-1;
    if(r0<0) r0=0;
    if(r1>rmax-1) r1=rmax-1;
    for(int r=r0;r<=r1;r++)for(int i=a0;i<=a1;i++){ char c=A[r][fl?w-1-i:i]; if(c!='.') return c; }
    return '.';
}
// ---- ALIVE: the portrait moves. hudFaceAnim picks an eye and a mouth sprite that replace the mood's for this picture (-1 = keep the mood's own). The portrait is redrawn only when the pick changes. ----
//   order of importance: a reaction to what just happened (hudRx, by mood event, about 1 second) > talking in a social > sleeping / washing > a blink every few seconds.
//   Eye ids: 0 SLEEPY 1 ROUND 2 HAPPY 3 WIDE 4 ANGRY 5 CUTE 6 CAT 7 DOT 8 LASHES.  Mouth ids: 0 FLAT 1 SMILE 2 OH 3 GRIN 4 SMIRK 5 FROWN 6 TONGUE 7 FANGS 8 KITTY.  mouth -2 = chewing.
static signed char hudOvE=-1, hudOvM=-1;
static const signed char hudRx[M_N][2]={
    [M_TRICK]={3,3}, [M_COMBO]={3,6}, [M_GRIND_ON]={-1,4}, [M_LAUNCH]={3,2}, [M_GOT_BOARD]={2,3}, [M_EAT]={-1,-2}, [M_RELIEVE]={2,1}, [M_SLEEP]={0,0},
    [M_SHOWER]={2,1}, [M_SOFA]={2,1}, [M_WANT]={3,3}, [M_SKILL]={3,3}, [M_PAY]={2,3}, [M_PROMO]={3,3}, [M_CHILL]={2,4},
    [M_BAIL]={3,2}, [M_HURT]={4,7}, [M_HURT_BIG]={3,7}, [M_BUMP]={3,2}, [M_ACCIDENT]={3,5}, [M_FAINT]={0,2}, [M_DIE]={7,2}, [M_FEAR]={3,2},
    [M_PASSOUT]={0,0}, [M_BROKE]={5,5}, [M_DEMOTE]={5,5}, [M_SPOOK]={3,2}, [M_SOAKED]={3,5} };
static void hudFaceAnim(int st){
    int e=-1, m=-1; unsigned t=(unsigned)lfr;
    if(fxRxT&&fxRxEv<M_N){ e=hudRx[fxRxEv][0]; m=hudRx[fxRxEv][1]; if(m==-2) m=((t>>3)&1)?2:1; }   // a reaction (eating: the mouth chews)
    else if(hhBubT){ static const signed char tk[4]={2,0,3,0}; m=tk[(t>>3)&3]; }                      // talking: the mouth opens and shuts
    else if(simAct==1){ e=0; m=0; } else if(simAct==2){ e=2; m=1; }                                    // asleep: eyes shut; washing: eyes closed and a smile
    if(e<0&&hudExpr[st][0]!=0&&hudExpr[st][0]!=2&&(t+53u)%220u<5u) e=0;                                 // a blink (not when the eyes are shut already)
    hudOvE=(signed char)e; hudOvM=(signed char)m;
}
static void hudFaceDraw(int x,int y,int st){   // x,y = top left of the 22 x 22 face area inside the portrait frame
    u16 skin=toneBy(skinTones[look[LK_SKIN]],slideEffS(look[LK_TONE])), hair=hudHairCol();
    u16 pal[10]={ RGB(3,3,6), RGB(31,31,31), RGB(29,12,16), shade(skin,11), toneBy(eyeTones[look[LK_EYECOL]%NSW],slideEffS(look[LK_EYETONE])),
                 RGB(((skin&31)+31)/2,(((skin>>5)&31)+8)/2,(((skin>>10)&31)+12)/2), RGB(6,6,8), shade(hair,10), RGB(4,5,9), RGB(13,22,31) };   // the face palette of setColors, unshaded, and a tear blue
    static const u8 ovl[22]={3,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,3};   // the head is an oval: columns cut off at the left (and right) of each row
    u16 jaw=shade(skin,13);
    for(int j=0;j<22;j++){ int c=ovl[j]; rect(x+c,y+j,22-2*c,1,j>=20?jaw:skin); }
    int hs=look[LK_HSTYLE]%NHAIR, hr=hudHairRows[hs];
    for(int j=0;j<22;j++){ int c=ovl[j]; if(j<hr) rect(x+c,y+j,22-2*c,1,hair); else if(hs==2&&j<20){ rect(x+c,y+j,2,1,hair); rect(x+20-c,y+j,2,1,hair); } }   // LONG hair falls down both sides
    const u8*e=hudExpr[st]; int eo=hudOvE>=0?hudOvE:e[0], mo=hudOvM>=0?hudOvM:e[1]; const Spr*es=&spr[eo], *ms=&spr[NEYE+mo];
    int br=e[2]?e[2]:look[LK_BROW], gl=look[LK_GLASS], no=look[LK_NOSE], ch=look[LK_CHEEK], baby=stage==AG_BABY;
    // The face sliders work as they do on the block (drawDeco): every feature is sampled through the same scale and shift, so eye size, spacing and height, mouth width
    // and height, brow height and nose height (and the master controller's double sliders) put the features where the creator does. The block's pixels are 7 x 6 per
    // eye and 15 x 6 per mouth, the portrait's art is 9 x 8 and 19 x 8, so shifts are scaled by 9/7 across and 4/3 down; sizes are ratios and carry over as they are.
    { char tr[8][9]; for(int j=0;j<8;j++)for(int i=0;i<9;i++) tr[j][i]=es->art[j][i];
      if(e[3]){ tr[6][1]='t'; tr[7][1]='t'; }   // a tear runs down the outer edge of each eye
      const char*ea[8]; for(int j=0;j<8;j++) ea[j]=tr[j];
      int en=slideEffS(look[LK_EYESZ])+(baby?1:0); en=en>0?2*en:en<0?en-1:0;
      int gw=7+en, gh=6+en; if(gw<1) gw=1; if(gh<1) gh=1; int kx=64*gw/7, ky=64*gh/6;
      int sv=slideEff(look[LK_EYEHT])*4/3, bsh=slideEff(look[LK_BROWHT])*4/3;
      for(int side=0;side<2;side++){   // left eye, then the right one mirrored (angry and sad eyes slope towards the nose)
        int ox=1+side*11, oy=3, sh=slideEff(look[LK_EYESP])*9/7; if(side) sh=-sh;
        for(int dy=-4;dy<12;dy++)for(int dx=-4;dx<13;dx++){
            int X=ox+dx, Y=oy+dy; if(X<0||X>21||Y<0||Y>21) continue;
            int a0,a1,r0,r1; hudRng(dx+sh,4,kx,&a0,&a1); hudRng(dy+sv,4,ky,&r0,&r1);
            char c=hudPick(ea,8,9,side,a0,a1,r0,r1);
            if(br>0&&br<=5){ char t=hudPick(brArt[br-1],2,9,side,a0,a1,r0+bsh,r1+bsh); if(t!='.') c=t; }
            if(gl>0&&gl<=3){ char t=hudPick(glArt[gl-1],8,9,side,a0,a1,r0,r1); if(t!='.') c=t; }
            if(c!='.') hudSprPlot(x+X,y+Y,&c,1,0,pal);
        }
      }
    }
    {   // the mouth, with the nose and cheeks around it
        int mw=slideEffS(look[LK_MOUTHW])-(baby?1:0), gw=15+mw*3; if(gw<1) gw=1; int kx=64*gw/15;
        int sv=slideEff(look[LK_MOUTHHT])*4/3, nsh=slideEff(look[LK_NOSEHT])*4/3;
        for(int dy=-7;dy<15;dy++)for(int dx=-7;dx<26;dx++){
            int X=2+dx, Y=11+dy; if(X<0||X>21||Y<0||Y>21) continue;
            int a0,a1,r0,r1; hudRng(dx,9,kx,&a0,&a1); hudRng(dy+sv,4,64,&r0,&r1);
            char c=hudPick(ms->art,8,19,0,a0,a1,r0,r1);
            if(c=='.'&&no>0&&no<=5) c=hudPick(noArt[no-1],2,19,0,a0,a1,dy+nsh,dy+nsh);
            if(c=='.'&&ch>0&&ch<=4) c=hudPick(chArt[ch-1],4,19,0,a0,a1,r0,r1);
            if(c!='.') hudSprPlot(x+X,y+Y,&c,1,0,pal);
        }
    }
}
static unsigned hudFaceKey(int st){   // everything the portrait draws from, so it is redrawn when the mood or the Sim (or their look) changes
    static const u8 ids[]={LK_SKIN,LK_TONE,LK_HCOL,LK_HTONE,LK_HSTYLE,LK_EYECOL,LK_EYETONE,LK_BROW,LK_GLASS,LK_NOSE,LK_CHEEK,LK_EYESZ,LK_EYESP,LK_EYEHT,LK_MOUTHW,LK_MOUTHHT,LK_BROWHT,LK_NOSEHT};
    unsigned h=(2166136261u^(unsigned)st)*16777619u^(unsigned)(stage*2+mcDbl());   // (the age and the double sliders change the face too)
    for(unsigned i=0;i<sizeof ids;i++) h=(h^look[ids[i]])*16777619u;
    return h|1u;
}
