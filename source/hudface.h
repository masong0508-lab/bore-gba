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
// MISSY has her own set (deadpan, half-lidded, a smirk instead of a grin; Daria, but foxy). Same row layout as hudExpr.
static const u8 hudExprMissy[5][4]={ {0,5,4,0}, {0,0,0,0}, {0,4,0,0}, {2,4,0,0}, {8,4,0,0} };   // SAD BORED OK HAPPY STOKED
static const u8 hudMsE[15]={0,1,2,1,4,0,6,7,8,9,10,11,12,13,14};   // her reactions: WIDE and CUTE eyes become ROUND and SLEEPY
static const u8 hudMsM[15]={0,4,2,4,4,5,4,4,8,9,10,11,12,13,14};   // SMILE, GRIN, TONGUE and FANGS all become a SMIRK
static int hudMissy(void){ return hhPName[0]=='M'&&hhPName[1]=='I'&&hhPName[2]=='S'&&hhPName[3]=='S'&&hhPName[4]=='Y'&&!hhPName[5]; }
static const u8* hudEx(int st){ return hudMissy()?hudExprMissy[st]:hudExpr[st]; }
static const u8 hudHairRows[NHAIR]={2,3,3,0,3,4,2,3,3, 4,3,3,0,3,3};   // rows of hair across the top of the portrait per hair style (CROP BOWL LONG BALD SPIKY AFRO FLAT TOP SIDE TAIL BUN BOB PONYTAIL PIGTAILS MOHAWK PIXIE CURLS)
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
    if(e<0&&hudEx(st)[0]!=0&&hudEx(st)[0]!=2&&(t+53u)%220u<5u) e=0;                                 // a blink (not when the eyes are shut already)
    if(hudMissy()){ if(e>=0&&e<NEYE) e=hudMsE[e]; if(m>=0&&m<NMOUTH) m=hudMsM[m]; }
    hudOvE=(signed char)e; hudOvM=(signed char)m;
}
static void hudHairX(int x,int y,int hs,u16 hair,const u8*ovl){   // the newer hair styles: what each adds around the top rows of the portrait (BOB PONYTAIL PIGTAILS MOHAWK PIXIE CURLS)
    u16 tie=RGB(29,12,16);
    switch(hs){
    case 9:   // BOB: a parted fringe, then hair framing the face down to the chin
        rect(x,y+4,7,1,hair); rect(x+15,y+4,7,1,hair);
        for(int j=5;j<20;j++){ int c=ovl[j], w=(j<8||j>=18)?4:3; rect(x+c,y+j,w,1,hair); rect(x+22-c-w,y+j,w,1,hair); }
        break;
    case 10:  // PONYTAIL: a swept fringe, and the tail tied at one side
        rect(x,y+3,15,1,hair); rect(x+20,y+4,2,1,tie); rect(x+20,y+5,2,9,hair);
        break;
    case 11:  // PIGTAILS: a parted fringe and a tied bunch down each side
        rect(x,y+3,8,1,hair); rect(x+14,y+3,8,1,hair);
        rect(x,y+5,2,1,tie); rect(x+20,y+5,2,1,tie); rect(x,y+6,2,9,hair); rect(x+20,y+6,2,9,hair);
        break;
    case 12:  // MOHAWK: a strip down the middle that narrows to a point
        rect(x+8,y,6,3,hair); rect(x+9,y+3,4,1,hair); rect(x+10,y+4,2,1,hair);
        break;
    case 13:  // PIXIE: a swept fringe and short sideburns
        rect(x,y+3,16,1,hair); rect(x,y+4,9,1,hair); rect(x,y+5,2,5,hair); rect(x+20,y+4,2,5,hair);
        break;
    case 14:  // CURLS: a scalloped fringe and lumpy sides
        for(int i=0;i<22;i++){ if(((i>>1)&1)==0) hudPx(x+i,y+3,hair); else hudPx(x+i,y+4,hair); }
        for(int j=5;j<12;j++){ int w=(j&2)?3:2; rect(x,y+j,w,1,hair); rect(x+22-w,y+j,w,1,hair); }
        break;
    }
}
static void faceDrawL(int x,int y,const u8*lk,int stg,const u8*e,int ovE,int ovM){   // anyone's face at 1x: x,y = top left of the 22 x 22 face; lk their look, stg their age, e an expression row (as hudExpr), ovE / ovM eye and mouth to use instead (-1: the row's)
    u16 skin=toneBy(skinTones[lk[LK_SKIN]],slideEffS(lk[LK_TONE])), hair=toneBy(hairTones[lk[LK_HCOL]%NSW],slideEffS(lk[LK_HTONE]));
    u16 pal[10]={ RGB(3,3,6), RGB(31,31,31), RGB(29,12,16), shade(skin,11), toneBy(eyeTones[lk[LK_EYECOL]%NSW],slideEffS(lk[LK_EYETONE])),
                 RGB(((skin&31)+31)/2,(((skin>>5)&31)+8)/2,(((skin>>10)&31)+12)/2), RGB(6,6,8), shade(hair,10), RGB(4,5,9), RGB(13,22,31) };   // the face palette of setColors, unshaded, and a tear blue
    static const u8 ovl[22]={3,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,3};   // the head is an oval: columns cut off at the left (and right) of each row
    u16 jaw=shade(skin,13);
    for(int j=0;j<22;j++){ int c=ovl[j]; rect(x+c,y+j,22-2*c,1,j>=20?jaw:skin); }
    int hs=lk[LK_HSTYLE]%NHAIR, hr=hudHairRows[hs];
    for(int j=0;j<22;j++){ int c=ovl[j]; if(j<hr) rect(x+c,y+j,22-2*c,1,hair); else if(hs==2&&j<20){ rect(x+c,y+j,2,1,hair); rect(x+20-c,y+j,2,1,hair); } }   // LONG hair falls down both sides
    if(hs>=9) hudHairX(x,y,hs,hair,ovl);   // the newer styles add their fringe, sides and tails
    int eo=ovE>=0?ovE:e[0], mo=ovM>=0?ovM:e[1]; const Spr*es=&spr[eyeSpr(eo)], *ms=&spr[mouthSpr(mo)];
    int br=e[2]?e[2]:lk[LK_BROW], gl=lk[LK_GLASS], no=lk[LK_NOSE], ch=lk[LK_CHEEK], baby=stg==AG_BABY;
    // The face sliders work as they do on the block (drawDeco): every feature is sampled through the same scale and shift, so eye size, spacing and height, mouth width
    // and height, brow height and nose height (and the master controller's double sliders) put the features where the creator does. The block's pixels are 7 x 6 per
    // eye and 15 x 6 per mouth, the portrait's art is 9 x 8 and 19 x 8, so shifts are scaled by 9/7 across and 4/3 down; sizes are ratios and carry over as they are.
    { char tr[8][9]; for(int j=0;j<8;j++)for(int i=0;i<9;i++) tr[j][i]=es->art[j][i];
      if(e[3]){ tr[6][1]='t'; tr[7][1]='t'; }   // a tear runs down the outer edge of each eye
      const char*ea[8]; for(int j=0;j<8;j++) ea[j]=tr[j];
      int en=slideEffS(lk[LK_EYESZ])+(baby?1:0); en=en>0?2*en:en<0?en-1:0;
      int gw=7+en, gh=6+en; if(gw<1) gw=1; if(gh<1) gh=1; int kx=64*gw/7, ky=64*gh/6;
      int sv=slideEff(lk[LK_EYEHT])*4/3, bsh=slideEff(lk[LK_BROWHT])*4/3;
      for(int side=0;side<2;side++){   // left eye, then the right one mirrored (angry and sad eyes slope towards the nose)
        int ox=1+side*11, oy=3, sh=slideEff(lk[LK_EYESP])*9/7; if(side) sh=-sh;
        short ca[17][2], ra[16][2];   // the art spans of each column and row (they only depend on dx or dy: worked out once, not per pixel)
        for(int dx=-4;dx<13;dx++){ int a0,a1; hudRng(dx+sh,4,kx,&a0,&a1); ca[dx+4][0]=(short)a0; ca[dx+4][1]=(short)a1; }
        for(int dy=-4;dy<12;dy++){ int r0,r1; hudRng(dy+sv,4,ky,&r0,&r1); ra[dy+4][0]=(short)r0; ra[dy+4][1]=(short)r1; }
        for(int dy=-4;dy<12;dy++)for(int dx=-4;dx<13;dx++){
            int X=ox+dx, Y=oy+dy; if(X<0||X>21||Y<0||Y>21) continue;
            int a0=ca[dx+4][0], a1=ca[dx+4][1], r0=ra[dy+4][0], r1=ra[dy+4][1];
            char c=hudPick(ea,8,9,side,a0,a1,r0,r1);
            if(br>0&&br<=5){ char t=hudPick(brArt[br-1],2,9,side,a0,a1,r0+bsh,r1+bsh); if(t!='.') c=t; }
            if(gl>0&&gl<=3){ char t=hudPick(glArt[gl-1],8,9,side,a0,a1,r0,r1); if(t!='.') c=t; }
            if(c!='.') hudSprPlot(x+X,y+Y,&c,1,0,pal);
        }
      }
    }
    {   // the mouth, with the nose and cheeks around it
        int mw=slideEffS(lk[LK_MOUTHW])-(baby?1:0), gw=15+mw*3; if(gw<1) gw=1; int kx=64*gw/15;
        int sv=slideEff(lk[LK_MOUTHHT])*4/3, nsh=slideEff(lk[LK_NOSEHT])*4/3;
        short ca[33][2], ra[22][2];   // (as for the eyes: the spans once per column and row)
        for(int dx=-7;dx<26;dx++){ int a0,a1; hudRng(dx,9,kx,&a0,&a1); ca[dx+7][0]=(short)a0; ca[dx+7][1]=(short)a1; }
        for(int dy=-7;dy<15;dy++){ int r0,r1; hudRng(dy+sv,4,64,&r0,&r1); ra[dy+7][0]=(short)r0; ra[dy+7][1]=(short)r1; }
        for(int dy=-7;dy<15;dy++)for(int dx=-7;dx<26;dx++){
            int X=2+dx, Y=11+dy; if(X<0||X>21||Y<0||Y>21) continue;
            int a0=ca[dx+7][0], a1=ca[dx+7][1], r0=ra[dy+7][0], r1=ra[dy+7][1];
            char c=hudPick(ms->art,8,19,0,a0,a1,r0,r1);
            if(c=='.'&&no>0&&no<=5) c=hudPick(noArt[no-1],2,19,0,a0,a1,dy+nsh,dy+nsh);
            if(c=='.'&&ch>0&&ch<=4) c=hudPick(chArt[ch-1],4,19,0,a0,a1,r0,r1);
            if(c!='.') hudSprPlot(x+X,y+Y,&c,1,0,pal);
        }
    }
}
static void hudFaceDraw(int x,int y,int st){ faceDrawL(x,y,look,stage,hudEx(st),hudOvE,hudOvM); }   // the HUD's mood portrait: the Sim you control, alive (x,y = top left of the 22 x 22 face area inside the portrait frame)
static unsigned hudFaceKey(int st){   // everything the portrait draws from, so it is redrawn when the mood or the Sim (or their look) changes
    static const u8 ids[]={LK_SKIN,LK_TONE,LK_HCOL,LK_HTONE,LK_HSTYLE,LK_EYECOL,LK_EYETONE,LK_BROW,LK_GLASS,LK_NOSE,LK_CHEEK,LK_EYESZ,LK_EYESP,LK_EYEHT,LK_MOUTHW,LK_MOUTHHT,LK_BROWHT,LK_NOSEHT};
    unsigned h=(2166136261u^(unsigned)st)*16777619u^(unsigned)(stage*2+mcDbl()+(hudMissy()?64:0));   // (the age and the double sliders change the face too)
    for(unsigned i=0;i<sizeof ids;i++) h=(h^look[ids[i]])*16777619u;
    return h|1u;
}
