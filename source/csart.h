// csart.h - CUTSCENE CAST ART (module 1): every figure in the in-game look: dark tousled hair, round face, game skin / hair / top colours, a dark outline, shading and highlights.
// Replaces csFig of cutscene.h (the old one stays as csFigV1, unused). Same call: csFig(x,y,who,pose,t): 34 px tall, feet at (x,y). Needs csR csD csLn csWv from cutscene.h.
static u16 csSh(u16 c,int k){ int r=(c&31)-k,g=((c>>5)&31)-k,b=((c>>10)&31)-k; if(r<0)r=0; if(g<0)g=0; if(b<0)b=0; return RGB(r,g,b); }   // darker
static u16 csLt(u16 c,int k){ int r=(c&31)+k,g=((c>>5)&31)+k,b=((c>>10)&31)+k; if(r>31)r=31; if(g>31)g=31; if(b>31)b=31; return RGB(r,g,b); }   // lighter
// cutscene redo 14 (step 2: BLENDING): when a beat changes a figure no longer snaps to the new pose. The eight pose numbers (lean, bob, both hands, legs, head) ease from where the
// figure WAS to the new pose over 12 frames, with a small overshoot (it settles into the pose, like a spring). Per figure (indexed by who); state lives in EWRAM.
typedef struct { short cur[8], from[8]; short lt, fn, bt, bn, x, x0, xt; u8 have, tw, bg; } CsPS;   // (bt: the beat frame the blend started; bn: the beat last drawn; x: where the figure stood last frame; x0 -> xt: a walk to a new spot; tw: walking last frame)
static CsPS csPS[6] EWRAM_BSS;
static int csFrameNo;
// cutscene redo 14 (step 3: SECONDARY MOTION): the hair strands and the skirt / coat hem trail behind the body and swing back past it (a small spring on the head's x), so a lean, a
// step or a dance move has weight. csHL[who]: the smoothed head x (1/16 px), its speed, the last frame it was updated.
typedef struct { short q, w, f; } CsHL;
static CsHL csHL[6] EWRAM_BSS;
static void csAliveReset(void){ for(int i=0;i<6;i++){ csPS[i].fn=-100; csPS[i].have=0; csPS[i].lt=0; csHL[i].f=-100; } csFrameNo=0; csHlag=0; }
static int csHairLag(int who,int hx){   // spring the head x; returns how many px the hair trails (negative: behind a head that moved right), -3 .. 3
    CsHL*h=&csHL[who]; int t16=hx*16;
    if(csFrameNo-h->f>3){ h->q=(short)t16; h->w=0; } else { int w=(h->w+(t16-h->q)/5)*3/4; h->w=(short)w; h->q=(short)(h->q+w); }
    h->f=(short)csFrameNo; int l=(h->q-t16+(h->q>=t16?8:-8))/16; return l>3?3:l<-3?-3:l;
}
static void csHairR(int x,int y,int w,int h,u16 c,int lag){ int a=h/3; csR(x,y,w,a,c); csR(x+lag/2,y+a,w,a,c); csR(x+lag,y+2*a,w,h-2*a,c); }   // a hair strand in three pieces: the tip trails
// cutscene redo 14 (step 4: EXPRESSIONS and REACTIONS): every face has a mood (csMd: 0 neutral, 1 sad, 2 happy, 3 angry, 4 shocked, 5 puzzled, 6 worried) that comes from the pose, the beat's
// effects, what the speaker is typing right now (? ! ...) and what the OTHER figure is doing. A listener reacts a few frames after the speaker's punctuation, comforts a crying friend
// (leans in, worried brows), smiles with a laughing one, and every figure flinches at a startling sound.
static int csPunc(const CsBeat*b,int n){   // the last '?', '!' or '...' within the first n typed characters, if it is recent (16 chars): 1 question, 2 exclamation, 3 trailing off, 0 none
    int pos=0, last=-99, kind=0, prev=0;
    for(int i=0;i<3&&b->t[i];i++){ const char*s=b->t[i]; for(int j=0;s[j]&&pos<n;j++,pos++){ int c=s[j]; if(c=='?'){ last=pos; kind=1; } else if(c=='!'){ last=pos; kind=2; } else if(c=='.'&&prev=='.'){ last=pos; kind=3; } prev=c; } }
    return n-last<=16?kind:0;
}
static void csBrows(int hx,int by,u16 hr,int md){   // the brows for a mood (G1: shared by the small sprites, Missy's too)
    if(md==1||md==6){ csR(hx-3,by+1,1,1,hr); csR(hx-2,by,1,1,hr); csR(hx+1,by,1,1,hr); csR(hx+2,by+1,1,1,hr); }                       // inner ends up
    else if(md==3){ csR(hx-3,by,1,1,hr); csR(hx-2,by+1,1,1,hr); csR(hx+1,by+1,1,1,hr); csR(hx+2,by,1,1,hr); }                         // inner ends down
    else if(md==5){ csR(hx-3,by,2,1,hr); csR(hx+1,by-2,2,1,hr); }                                                                     // one brow up
    else { csR(hx-3,by,2,1,hr); csR(hx+1,by,2,1,hr); }
}
static void csEyesS(int hx,int hy,int bl,u16 sk,u16 hr,int md){   // eyes and brows of the small sprite (not Missy: her glasses keep their own)
    u16 dk=RGB(3,2,3), wh=RGB(30,30,31); int lk=csLook, by=hy-1-csEm;
    if(md==4&&!bl){ csR(hx-3,hy,2,2,wh); csR(hx+1,hy,2,2,wh); csR(hx-3+(lk>0),hy+1,1,1,dk); csR(hx+1+(lk>0),hy+1,1,1,dk); by--; }    // wide eyes, brows up
    else if(md==2&&!bl){ csR(hx-3,hy+1,2,1,dk); csR(hx+1,hy+1,2,1,dk); }                                                              // smiling eyes: two dashes
    else { csR(hx-2+lk,hy+1,1,1,bl?sk:dk); csR(hx+1+lk,hy+1,1,1,bl?sk:dk); }
    csBrows(hx,by,hr,md);
}
static void csMouthS(int hx,int hy,int who,int open,int md){   // the small sprite's mouth for a mood
    u16 lp=who==CA_MAME?RGB(22,9,9):RGB(18,6,6), od=RGB(18,2,3);
    if(open){ csR(hx-1,hy+4,2,csMh>4?2:1,od); if(md==2){ csR(hx-2,hy+4,1,1,lp); csR(hx+1,hy+4,1,1,lp); } return; }
    switch(md){
    case 1: case 6: csR(hx-1,hy+4,2,1,lp); csR(hx-2,hy+5,1,1,lp); csR(hx+1,hy+5,1,1,lp); break;           // a frown
    case 2: csR(hx-1,hy+5,2,1,lp); csR(hx-2,hy+4,1,1,lp); csR(hx+1,hy+4,1,1,lp); break;                   // a smile
    case 3: csR(hx-1,hy+4,2,1,od); csR(hx-2,hy+5,1,1,lp); csR(hx+1,hy+5,1,1,lp); break;                   // tight, down at the corners
    case 4: csR(hx-1,hy+4,2,2,od); break;                                                                  // a small O
    case 5: csR(hx-1,hy+5,2,1,lp); csR(hx+1,hy+4,1,1,lp); break;                                          // crooked
    default: if(who==CA_MISSY){ csR(hx-1,hy+4,3,1,lp); csR(hx+2,hy+3,1,1,lp); }                           // Missy's deadpan smirk
             else { csR(hx-1,hy+4,2,1,lp); if(who==CA_MAME){ csR(hx-2,hy+3,1,1,lp); csR(hx+1,hy+3,1,1,lp); } }
    }
}
// cutscene redo 14 (step 1: ELBOWS): arms are two segments with a real elbow. The elbow bulges outward / down like a relaxed arm, and a raised hand folds the arm up.
static int csIq(int v){ int w=0; while((w+1)*(w+1)<=v) w++; return w; }
static void csLimb(int x0,int y0,int x1,int y1,u16 fill,u16 ol){   // a 2 px wide, outlined bone
    int dx=x1-x0, dy=y1-y0; if(dx<0) dx=-dx; if(dy<0) dy=-dy;
    if(dy>=dx){ csLn(x0-1,y0,x1-1,y1,ol); csLn(x0+2,y0,x1+2,y1,ol); csLn(x0,y0,x1,y1,fill); csLn(x0+1,y0,x1+1,y1,fill); }
    else      { csLn(x0,y0-1,x1,y1-1,ol); csLn(x0,y0+2,x1,y1+2,ol); csLn(x0,y0,x1,y1,fill); csLn(x0,y0+1,x1,y1+1,fill); }
}
static void csArm(int sx,int sy,int hx,int hy,int side,u16 ac,u16 ol,u16 sk){   // side -1 left, +1 right; shoulder (sx,sy) to the hand at (hx,hy)
    int ax=sx+(side<0?1:-2), bx=hx+(side<0?0:-1), dx=bx-ax, dy=hy-sy, d=csIq(dx*dx+dy*dy), ex=ax+dx/2, ey=sy+dy/2;
    if(d>0&&d<10){ int px=-dy, py=dx; if(px*side<0||(px==0&&py<0)){ px=-px; py=-py; }     // the bulge points away from the body, or down
        int h=csIq(25-d*d/4)*3/5; ex+=px*h/d; ey+=py*h/d; }
    csLimb(ax,sy,ex,ey,ac,ol); csLimb(ex,ey,bx,hy,ac,ol);
    csR(hx-1,hy-1,2,2,sk);                                                                  // the hand
}
// cutscene redo 14 (step 6: WALK AND RUN CYCLE): while a figure walks or runs the legs are two-segment limbs (thigh, knee, shin) that swing in step, the swinging foot lifts and the knee
// bends toward the way they go; the arms counter-swing (opposite hand to the forward foot) and their hands rise with the elbows; the body dips as the feet spread and rises as they pass.
static int csStepLift(int t,int per,int mx){ int p=((t%per)+per)%per, h=per/2; if(p>=h) return 0; int q=p<h/2?p:h-p; return q*mx*2/h; }   // 0 .. mx .. 0 over half a cycle, then 0
static void csFig(int x,int y,int who,int pose,int t){   // t: the motion clock (csFt: on the tune's beat when one plays); csBT: frames since the beat began, for what happens once (walk in, leave, slump, the shock's jolt)
    //                         -    MISSY        MAMESY       DEX          HAL          OKAFOR     (the game's own skinTones / hairTones / topTones / botTones)
    static const u16 SKc[6]={0,RGB(24,16,10),RGB(24,16,10),RGB(30,23,17),RGB(19,12,7),RGB(13,8,5)};
    static const u16 HRc[6]={0,RGB(5,3,2),RGB(14,8,4),RGB(5,3,2),RGB(14,8,4),RGB(5,3,2)};
    static const u16 CLc[6]={0,RGB(8,10,26),RGB(8,20,22),RGB(8,9,14),RGB(30,16,4),RGB(29,29,30)};   // blazer, teal top, navy suit jacket, hi-vis vest, white coat
    static const u16 BTc[6]={0,RGB(7,8,15),RGB(9,13,23),RGB(4,4,7),RGB(17,14,8),RGB(8,12,20)};   // pencil skirt, jeans, suit trousers, work trousers, scrub trousers
    u16 sk=SKc[who], hr=HRc[who], cl=CLc[who], bt=BTc[who], dk=RGB(3,2,3), ol=RGB(2,1,4);
    int tv=0, tgd=1;   // TRAVEL: a figure who stood somewhere else in the last beat (same backdrop, no fade) walks there, or runs in a RUN beat, instead of jumping to the new spot
    { CsPS*p=&csPS[who]; int can=!csEnt&&pose!=CP_LIE&&pose!=CP_STIR&&pose!=CP_CLIMB&&pose!=CP_FLAIL&&pose!=CP_WALK&&pose!=CP_LEAVE&&csBgNow!=CB_MIRROR&&!(csFxNow&CF_FADEIN);
      if(!can||csFrameNo-p->fn>3||p->bg!=csBgNow){ p->x0=p->xt=(short)x; } else if(p->bn!=csBN){ p->x0=p->x; p->xt=(short)x; }
      int d=p->xt-p->x0;
      if(d&&p->xt==x){ int sp=pose==CP_RUN?7:4, T=((d<0?-d:d)*2+sp-1)/sp;   // half pixels a frame: a walk is 2 px, a run 3.5
          if(csBT<T){ x=p->x0+d*csBT/T; tv=T>=6?1:2; tgd=d>0?1:-1; if(tv==1) csDir=tgd; } }   // (a step of a pixel or two just slides; a real walk faces the way it goes)
      p->x=(short)x; p->bg=(u8)csBgNow; }
    int ent=csEnt; if(pose==CP_LIE||pose==CP_STIR) ent=0; x+=ent;   // C8: stepping in from / out toward the edge of the picture (csEnt: px from the figure's spot, csEntD: the way it walks)
    int mv=0, gait=0, gd=1, gs=0; if(pose==CP_WALK){ int bt=csBT; if(bt<60){ gd=x>120?-1:1; x+=(x>120?60-bt:bt-60); mv=1; } } else if(pose==CP_LEAVE){ int bt=csBT; if(bt>45){ gd=x>120?1:-1; x+=(x>120?bt-45:45-bt); mv=1; } }   // cutscene redo 10: walk in from, and out toward, the nearer side
    int dress=who==CA_MISSY;   /* (Mamesy wears a top and jeans, like in the game) */
    int lmd=0, jb=0, nod=0, lbl=0;
    if(pose==CP_LIE||pose==CP_STIR){ csPS[who].fn=-100;   // lying down: no blend into or out of it (a different drawing), but G2: she reacts
        if(csStart>0){ lmd=4; jb=-((csStart+2)/4); } else if(csOth==CP_CRY||csOth==CP_HEAD||csOth==CP_SLUMP||(csFxNow&CF_SHAKE)) lmd=6; else if(csOth==CP_LAUGH||csOth==CP_DANCE) lmd=2; else if(csOth==CP_SHOCK) lmd=4;
        { int ph=(t+who*13)%96; nod=(csTalking&&csSpk&&csSpk!=who&&ph<10&&(ph/3)%2==0); }   // a nod when someone talks to her
        lbl=((t+who*23)%110)<4; csMd=lmd; }
    if(pose==CP_LIE||pose==CP_STIR){ int st=pose==CP_STIR, lf=st?(csWv(t,30)+8)/3:0, br=csWv(t,100)>4; csR(x-19,y-1,36,1,RGB(2,1,3)); y+=jb; csR(x-11,y-7,18,7,ol); csR(x-10,y-6,17,6,cl); csR(x-10,y-6,17,1,csLt(cl,5)); if(br) csR(x-8,y-8,13,1,cl); csR(x+7,y-5-lf,10,3,ol); csR(x+7,y-4-lf,9,2,dress?sk:bt); if(st) csR(x+17,y-5-lf+((t>>2)&1),1,2,sk);   // (cutscene redo 10: she breathes; STIR lifts her hand and her fingers move)
        { int hn=nod?1:0; csD(x-14,y-4-hn,5,ol); csD(x-14,y-4-hn,4,sk); csR(x-19,y-9-hn,6,6,hr); csR(x-18,y-10-hn,3,1,csLt(hr,7)); csR(x-15,y-5-hn,1,1,lbl?sk:dk);   // G2: the head nods, the eye blinks
            if(lmd==4) csR(x-13,y-4-hn,2,2,RGB(18,2,3)); else if(lmd==6){ csR(x-13,y-3-hn,2,1,RGB(24,8,8)); csR(x-15,y-6-hn,2,1,hr); } else if(lmd==2){ csR(x-13,y-3-hn,2,1,RGB(24,8,8)); csR(x-14,y-4-hn,1,1,RGB(24,8,8)); } else csR(x-13,y-3-hn,2,1,RGB(24,8,8)); } if(csCz>=384) csFaceBig(x-14,y-6,who,CP_LIE,t,0,SKc[who],HRc[who]); return; }
    int lean=0, bob=0, lh=-6, lv=8, rh=6, rv=8, ls=0, hd=0, open=0, md=0;
    switch(pose){
    case CP_SWAY:  lean=csWv(t,70)/2; lh=-7+csWv(t,50)/3; rh=7-csWv(t,50)/3; ls=csWv(t,70)/4; break;
    case CP_DANCE: bob=(csWv(t,16)+8)/6; lean=csWv(t,40)/2; lh=-9; lv=-9+csWv(t,16)/2; rh=9; rv=-9-csWv(t,16)/2; ls=csWv(t,16)/3; break;
    case CP_SING:  lean=csWv(t,60)/4; rh=2; rv=-8; lh=-10; lv=-3+csWv(t,30)/3; open=1; break;
    case CP_HEAD:  lean=-1; hd=2; lh=-4; lv=-9; rh=4; rv=-9; break;
    case CP_RUN:   gait=2; gd=csDir; gs=csWv(t,10); lean=3; lh=-5-gs*3/4; rh=5+gs*3/4; lv=rv=3-(gs<0?-gs:gs)/3; bob=(gs<0?-gs:gs)/4-1; ls=0; break;   // step 6: arms pump against the legs
    case CP_CLIMB: lh=-4; rh=4; lv=-8+csWv(t,24)/2; rv=-8-csWv(t,24)/2; ls=csWv(t,24)/2; break;
    case CP_FLAIL: lean=csWv(t,8)/3; lh=-9; lv=-8+csWv(t,6); rh=9; rv=-8-csWv(t,6); ls=csWv(t,6)/2; break;
    // cutscene redo 10: TALK gestures and the mouth moves while the caption types; LAUGH bounces; CRY hides the face and heaves; POINT at the other figure; SHOCK jolts back with the arms up;
    // WALK / LEAVE step in or out; SLUMP sinks and hangs the head
    case CP_TALK:  lean=csWv(t,50)/6; lh=-7; rh=8; rv=-4+csWv(t,14)/3; open=csTalking&&((t>>2)&1); break;
    case CP_LAUGH: bob=(csWv(t,8)+8)/5; lean=csWv(t,8)/5; hd=-1; lh=-4; lv=-2; rh=4; rv=-2; open=1; break;
    case CP_CRY:   { int w=csWv(t,(t%48)<24?6:20), bu=(t%48)<24; if(bu){ bob=-((w+8)/8); hd=3+(w+8)/8; } else { bob=(w+8)/10; hd=3; } lean=-1; lh=-2; lv=rv=-6-(bu&&w>4); rh=2; } break;   // step 7: sobs come in bursts: the shoulders heave up and the head is pulled down, then slow breaths
    case CP_POINT: lean=csDir*2; if(csDir>0){ rh=13; rv=-5; lh=-6; } else { lh=-13; lv=-5; rh=6; } open=csTalking&&((t>>2)&1); break;
    case CP_SHOCK: bob=csBT<6?-2:0; lean=-csDir*3; hd=-1; ls=2; lh=-9; lv=-12; rh=9; rv=-12; open=1; break;
    case CP_WALK: case CP_LEAVE: if(mv){ gait=1; gs=csWv(t,14); lean=1; lh=-5-gs/2; rh=5+gs/2; lv=rv=8-(gs<0?-gs:gs)/2; bob=((gs<0?-gs:gs)+4)/8; ls=0; } break;   // step 6
    case CP_SLUMP: bob=csBT/4>7?7:csBT/4; hd=3; lean=-1; break;
    }
    // cutscene redo 13 - ALIVE: everyone breathes and shifts their weight; whoever is speaking leans in, nods, gestures with BOTH hands and moves the mouth in syllables
    // (not just the TALK pose); the listener nods along; the eyes glance at the other person and now and then dart away. Layered on top of whatever the pose set.
    csLook=csDir; csEm=0; csMh=5;
    { int spk=csTalking&&csSpk==who, lis=csTalking&&csSpk&&csSpk!=who, syl=t>>2;
      int calm=(pose==CP_STAND||pose==CP_HEAD||pose==CP_SWAY||pose==CP_TALK||pose==CP_POINT||pose==CP_SHOCK||pose==CP_SLUMP);
      if(calm) bob+=(csWv(t+who*19,72)+8)/10;                                                                       // the chest rises a pixel every ~1.2 s
      if(pose==CP_STAND){ lean+=csWv(t+who*31,150)/6; lv+=(csWv(t+who*7,90)+8)/12; rv+=(csWv(t+who*11,110)+8)/12; }   // weight shifts, hands drift
      if(spk&&calm){
          int g=csWv(t+who*9,52), g2=csWv(t+who*9+26,70); if(g<0) g=0; if(g2<0) g2=0;                             // two hands, out of step: emphasis strokes
          if(pose!=CP_POINT&&pose!=CP_SHOCK) lean+=csDir;                                                         // leans toward who they are talking to
          if(((t>>3)&3)==0) hd+=1;                                                                                // a nod on the beat
          if(pose==CP_STAND||pose==CP_SWAY){ rv-=g*5/4; rh+=g/3; lv-=g2*5/4; lh-=g2/3; }
          if(pose==CP_TALK){ lv-=g2; lh-=g2/4; rv-=g/2; }
          if(g>4||g2>4) csEm=1;
          if(pose!=CP_SHOCK){ open=(syl*syl+syl/3)%5<3; csMh=3+(syl*5)%4; }                                          // syllables, not a square wave
      } else if(lis&&calm){ int ph=(t+who*13)%96; if(ph<10&&(ph/3)%2==0) hd+=1; }                                    // listener: a double nod now and then
      { int dt=(t/41+who*3)%5; if(dt==0) csLook=-csDir; else if(dt==1) csLook=0; }                               // glances away, then back
    }
    if(ent){ mv=1; gait=1; gd=csEntD; gs=csWv(t,14); lean=gd; lh=-5-gs/2; rh=5+gs/2; lv=rv=8-(gs<0?-gs:gs)/2; bob=((gs<0?-gs:gs)+4)/8; ls=0; }   // C8: the walk cycle while stepping in or out
    if(tv==1&&!ent&&pose!=CP_RUN){ mv=1; gait=1; gd=tgd; gs=csWv(t,14); lean=tgd; lh=-5-gs/2; rh=5+gs/2; lv=rv=8-(gs<0?-gs:gs)/2; bob=((gs<0?-gs:gs)+4)/8; ls=0; }   // the same walk on the way to a new spot (a RUN beat runs there)
    { CsPS*p=&csPS[who]; short v[8]={lean,bob,lh,lv,rh,rv,ls,hd};                                   // BLEND from where the figure was at the end of the last beat (or of the walk to its new spot)
      int nw=tv==1&&!ent;
      if(csFrameNo-p->fn>3) p->have=0; else if(p->bn!=csBN||(p->tw&&!nw)){ for(int i=0;i<8;i++) p->from[i]=p->cur[i]; p->have=1; p->bt=(short)csBT; }   // (beat time, not csFt's: with a tune playing that one never starts again)
      { int bt=csBT-p->bt; if(p->have&&bt>=0&&bt<12){ static const signed char ez[12]={0,5,9,12,14,16,17,17,17,16,16,16}; int k=ez[bt]; for(int i=0;i<8;i++) v[i]=(short)(p->from[i]+(v[i]-p->from[i])*k/16);
          lean=v[0]; bob=v[1]; lh=v[2]; lv=v[3]; rh=v[4]; rv=v[5]; ls=v[6]; hd=v[7]; } }
      for(int i=0;i<8;i++) p->cur[i]=v[i]; p->lt=(short)t; p->fn=(short)csFrameNo; p->bn=(short)csBN; p->tw=(u8)nw; }
    { int spk2=csTalking&&csSpk==who, lis2=csTalking&&csSpk&&csSpk!=who, shake=(csFxNow&CF_SHAKE)!=0;
      int sad=(pose==CP_HEAD||pose==CP_CRY||pose==CP_SLUMP), ok=(pose==CP_STAND||pose==CP_SWAY||pose==CP_TALK||pose==CP_POINT||pose==CP_WALK||pose==CP_LEAVE);   // ok: poses that can react
      int os=(csOth==CP_HEAD||csOth==CP_CRY||csOth==CP_SLUMP||csOth==CP_LIE), oh=(csOth==CP_LAUGH||csOth==CP_DANCE||csOth==CP_SING), ox=(csOth==CP_SHOCK||csOth==CP_FLAIL);
      md=0;
      if(sad) md=1; else if(pose==CP_LAUGH||pose==CP_DANCE||pose==CP_SING) md=2; else if(pose==CP_SHOCK||pose==CP_FLAIL||(csFxNow&CF_SICK)) md=4;
      else if(ok){
          if(csStart>0) md=4;                                                                  // a loud noise
          else if(spk2){ if(shake) md=3; else if(csPuncS==1) md=5; else if(csPuncS==3) md=6; }  // angry when heated, puzzled at a question, uncertain trailing off
          else { if(os||shake) md=6; else if(oh) md=2; else if(ox) md=4; else if(lis2&&csPuncL==2) md=4; else if(lis2&&csPuncL==1) md=5; else if(lis2&&csPuncL==3) md=6; }
      }
      if(ok){ if(spk2&&csPuncS==2) csEm=1;                                                     // an exclamation: brows up
          if(csStart>0){ int s=csStart; bob-=(s+2)/4; lean-=csDir*((s+3)/6); lv-=s/3; rv-=s/3; lh-=s/6; rh+=s/6; }   // flinch: jump, recoil, hands up
          if(os&&!spk2){ lean+=csDir; hd+=1; }                                                  // leans in toward someone who is hurting
          if(lis2&&csPuncL==2){ lean-=csDir; bob-=1; }                                          // a small recoil at a shout
          if(lis2&&csPuncL==1) lean+=csDir; }                                                   // leans in at a question
      csMd=md; }
    int sy=y-21+bob, hx=x+lean, hy=y-29+bob+hd; u16 lc=dress?sk:bt; int bl=((t+who*23)%110)<4;   // bl: a blink every ~2 s (cutscene redo 10)
    int cry=pose==CP_CRY||((pose==CP_HEAD||pose==CP_SLUMP)&&(csFxNow&CF_SHAKE)&&!(csFxNow&CF_SICK)), lau=pose==CP_LAUGH; if(cry||lau) bl=1;   // step 7: the small sprite's crying and laughing detail (eyes shut)
    int fem=(who==CA_MISSY||who==CA_MAME), ax=fem?5:6;   // the build: the women slimmer through the shoulders, the men broader (the arms hang from the edge of the shoulders)
    int lag=csHairLag(who,hx); csHlag=lag;                                                                              // how far the hair and the hem trail
    csR(x-6,y,12,1,RGB(2,1,3));                                                                                       // the floor shadow
    if(gait){ int per=gait==2?10:14, amp=gait==2?6:4, mx=gait==2?4:2; int a0=gd>0?t:t+per/2, a1=gd>0?t+per/2:t;   // step 6: the walk / run legs. a0 / a1: when the left / right foot lifts
        for(int k=0;k<2;k++){ int sg=k?-1:1, hxp=x+(k?2:-2), fx=x+(k?3:-3)+sg*gs*amp/8, fy=y-2-csStepLift(k?a1:a0,per,mx), hy2=y-12, dx=fx-hxp, dy=fy-hy2, d=csIq(dx*dx+dy*dy), kx=hxp+dx/2, ky=hy2+dy/2;
            if(d>0&&d<12){ int px=dy, py=-dx; if(px*gd<0){ px=-px; py=-py; } int h=csIq(36-d*d/4)*3/4; kx+=px*h/d; ky+=py*h/d; }   // the knee bulges the way they are going
            csLimb(hxp,hy2,kx,ky,lc,ol); csLimb(kx,ky,fx,fy,lc,ol);
            csR(gd>0?fx-2:fx-3,fy,5,2,dk); csR(gd>0?fx-1:fx-3,fy,2,1,csLt(dk,5)); } }                                    // shoes point the way they go
    else {
    csR(x-4+ls,y-12,3,12,ol); csR(x+1-ls,y-12,3,12,ol); csR(x-3+ls,y-11,2,11,lc); csR(x+1-ls,y-11,2,11,lc);               // legs: a third of the figure, trousers or jeans (bare under Missy's skirt)
    csR(x-5+ls,y-2,4,2,dk); csR(x+1-ls,y-2,4,2,dk); csR(x-4+ls,y-2,2,1,csLt(dk,5)); }                                      // shoes
    u16 ac=(who==CA_CREW)?RGB(14,14,16):cl;                                                                // arms: a sleeve (Hal: a grey work shirt under the vest)
    csArm(x+lean-ax,sy,x+lean+lh,sy+lv,-1,ac,ol,sk); csArm(x+lean+ax,sy,x+lean+rh,sy+rv,1,ac,ol,sk);                    // arms: shoulder, elbow, hand
    for(int i=0;i<11;i++){ int w=i<3?(fem?10:12):i<7?(fem?9:11):(fem?8:10), cx=x+lean*(11-i)/11, yy=y-22+i+bob;           // the body: straight, shoulders to hips, with an outline and a shaded side
        u16 rc=(who==CA_MISSY&&i>=9)?bt:cl;                                                                              // Missy: the blazer ends at the hip, the skirt starts
        csR(cx-w/2-1,yy,w+2,1,ol); csR(cx-w/2,yy,w,1,rc); csR(cx-w/2,yy,2,1,csSh(rc,4)); csR(cx+w/2-2,yy,1,1,csLt(rc,3));
        if(who==CA_MISSY){ if(i<4) csR(cx-(i<2?1:0),yy,i<2?3:1,1,WHITE); if(i==6) csR(cx,yy,1,1,csLt(cl,9)); }          // a white blouse in the V of the blazer, one button
        if(who==CA_MAME){ if(i<2) csR(cx-1,yy,3,1,csSh(sk,2)); if(i==10) csR(cx-w/2,yy,w,1,csSh(cl,3)); }                 // a round neck, a plain hem
        if(who==CA_HOST){ if(i>=1&&i<7){ csR(cx-2,yy,1,1,csLt(cl,5)); csR(cx+2,yy,1,1,csLt(cl,5)); csR(cx-1,yy,1,1,WHITE); csR(cx+1,yy,1,1,WHITE); } if(i<2) csR(cx-1,yy,3,1,WHITE); if(i>=1&&i<8) csR(cx,yy,1,1,RGB(24,4,6)); }   // Dex: lapels, white shirt, red tie
        if(who==CA_DOC){ if(i<2) csR(cx-1,yy,3,1,RGB(8,18,20)); if(i>=2&&i<10) csR(cx,yy,1,1,csSh(cl,10)); if(i==4||i==5) csR(cx-w/2+1,yy,2,1,csSh(cl,8)); }   // Okafor: scrubs at the neck, the coat seam, a pocket
        if(who==CA_CREW){ if(i<2) csR(cx-1,yy,3,1,RGB(14,14,16)); if(i>1&&i<10){ csR(cx-w/2+2,yy,1,1,RGB(31,30,16)); csR(cx+w/2-3,yy,1,1,RGB(31,30,16)); } }   // Hal: the grey shirt, the reflective vest stripes
    }
    if(who==CA_DOC) for(int k=0;k<7;k++){ int yy=y-11+k, xx=x+lag*(k+1)/7; csR(xx-6,yy,12,1,ol); csR(xx-5,yy,10,1,cl); csR(xx-5,yy,2,1,csSh(cl,4)); csR(xx,yy,1,1,csSh(cl,10)); }   // the white coat hangs to the knee
    if(who==CA_MISSY) for(int k=0;k<7;k++){ int yy=y-11+k, xx=x+lag*(k+1)/7; csR(xx-5,yy,10,1,ol); csR(xx-4,yy,8,1,bt); csR(xx-4,yy,1,1,csSh(bt,2)); if(k==6) csR(xx-4,yy,8,1,csLt(bt,5)); }   // a straight knee-length skirt
    if(csCz>=384){ csFaceBig(hx,hy+2,who,pose,t,open,sk,hr); return; }
    if(who==CA_MAME){ csHairR(hx-7,hy-1,3,15,ol,lag); csHairR(hx+4,hy-1,3,15,ol,lag); csHairR(hx-6,hy,2,14,hr,lag); csHairR(hx+4,hy,2,14,hr,lag); csR(hx-6+lag,hy+9,1,3,csLt(hr,5)); csR(hx+5+lag,hy+9,1,3,csLt(hr,5)); }      // long hair behind
    if(who==CA_MISSY){ csHairR(hx-6,hy-1,2,10,ol,lag); csHairR(hx+5,hy-1,2,10,ol,lag); csHairR(hx-6,hy,2,9,hr,lag); csHairR(hx+4,hy,2,9,hr,lag); }       // the bob, to the chin
    csD(hx,hy+1,6,ol); csD(hx,hy,5,hr); csD(hx,hy+2,4,sk);                                                                 // head: outline, hair, face
    if(who!=CA_MAME){ csR(hx-2,hy-4,2,1,csLt(hr,8)); } else { csR(hx-3,hy-2,7,2,hr); csR(hx-4,hy-1,2,4,hr); csR(hx+3,hy-1,2,3,hr); csR(hx+1,hy-4,2,1,csLt(hr,8)); }                       // the tousled tufts and a shine (the in-game hair)
    csR(hx-4,hy+4,1,1,RGB(28,12,12)); csR(hx+4,hy+4,1,1,RGB(28,12,12));                                                   // blush
    if(who==CA_MISSY){ u16 gl=RGB(9,9,12); int gx=hx, gy=hy+1; csR(gx-4,gy-1,3,1,gl); csR(gx-4,gy+1,3,1,gl); csR(gx-4,gy,1,1,gl); csR(gx-2,gy,1,1,gl); csR(gx+1,gy-1,3,1,gl); csR(gx+1,gy+1,3,1,gl); csR(gx+1,gy,1,1,gl); csR(gx+3,gy,1,1,gl); csR(gx-1,gy,2,1,gl); csR(gx-3,gy,1,1,bl?sk:dk); csR(gx+2,gy,1,1,bl?sk:dk); csR(gx-3,gy-4,7,1,hr); csR(gx-3,gy-3,7,2,sk); csBrows(gx,(md==0&&!csEm)?gy-2:gy-3,hr,md); }   // small round glasses with a bridge, flat sleepy eyes, a thin fringe, and (G1) brows in the forehead gap above the glasses
    else csEyesS(hx,hy,bl,sk,hr,md);                    // eyes, brows
    if(who==CA_HOST) csR(hx-4,hy-3,8,2,hr);                                                                               // Dex: swept fringe
    if(who==CA_CREW){ csR(hx-6,hy-3,12,3,RGB(31,31,28)); csR(hx-7,hy-1,14,1,RGB(24,24,22)); csR(hx-2,hy-4,4,1,RGB(31,31,31)); }   // Hal: the hard hat
    if(who==CA_DOC){ csLn(hx-3,hy+6,hx,hy+10,RGB(22,22,24)); csLn(hx+3,hy+6,hx,hy+10,RGB(22,22,24)); csD(hx,hy+11,1,RGB(26,26,28)); }   // Okafor: the stethoscope
    csMouthS(hx,hy,who,open,md);
    if(cry||lau){ u16 tr=RGB(14,22,31), tl=RGB(24,28,31), od=RGB(18,2,3), wh=RGB(30,30,31);   // step 7: eyes, tears and mouth of the small sprite
        if(who!=CA_MISSY){ csR(hx-3,hy,2,2,sk); csR(hx+1,hy,2,2,sk);
            if(lau){ csR(hx-3,hy+1,1,1,dk); csR(hx-2,hy,1,1,dk); csR(hx+1,hy,1,1,dk); csR(hx+2,hy+1,1,1,dk); }                  // laughing: two upturned arcs (^ ^)
            else { csR(hx-3,hy+1,2,1,dk); csR(hx+1,hy+1,2,1,dk); } }                                                              // crying: scrunched shut
        if(cry){ int ph=(t*2)%14; for(int e=0;e<2;e++){ int tx=e?hx+2:hx-3; csR(tx,hy+2,1,1,tl); csR(tx,hy+2+ph/3,1,1,tr); if(ph>6) csR(tx,hy+2+ph/3-1,1,1,tr); }   // two tears run down the cheeks and start again
            if((t>>1)&1) csR(hx-1,hy+5,2,1,RGB(18,6,6)); else csR(hx-1,hy+4,2,1,od); }                                           // the mouth quivers
        if(lau){ csR(hx-2,hy+4,4,((t>>1)&1)?3:2,od); csR(hx-1,hy+4,2,1,wh);                                                       // a wide laugh: teeth on top, the jaw bobbing
            if((t%48)<14) csR(hx+3,hy+2,1,1,tr); } }                                                                              // and a tear of joy now and then
}
