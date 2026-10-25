// selfwill.h - YOUR SIM ALONE (OPTIONS > SIM > YOUR SIM ALONE: OFF / LOW / HIGH). Put the pad down and your Sim stops being a statue:
// it looks after itself the way a household Sim with free will does (house.h), and it talks to people without you opening a menu.
// Include AFTER house.h (hhPlan, hhNearest, socDo, uTile ...). lifeStep (main.c) calls swPre() first thing; the walking goes through
// feelWalk's swUx / swUy, so it is the same body, speed and walls as when you steer.
//
//  QUIET   steps since the pad was last touched (swQuiet). LOW waits 25 s, HIGH 10 s. ANY button gives the Sim back at once, and it waits
//          a good while longer before it tries again (swCool).
//  PICK    the most pressing need scores like the household's (how low it is, squared). A low MOOD changes the order: glum (SAD) or bored
//          lifts comfort things (a snack, the sofa, the TV) and company; lonely goes to talk. With nothing pressing it now and then
//          wanders over to somebody anyway (HIGH more than LOW). It never starts a fight and never proposes: those stay yours.
//  WALK    hhPlan fills swS.path (swS is a spare HhSim that only keeps the path); each step the pad is faked toward the next tile.
//  USE     next to the furniture it presses R like you would (lnear says what is in reach); next to a Sim it calls socDo, the same
//          call the pie menu makes.  Something in the way, a Sim in reach of the furniture, or no route: it gives that up for a while.
//
// TUNING
#define SW_LOW_WAIT  1500   // LOW: steps of quiet before it acts (25 s)
#define SW_HI_WAIT    600   // HIGH: 10 s
#define SW_GAP        150   // steps between two things it does (2.5 s)
#define SW_CHAT_GAP  1500   // after a chat: 25 s before the next walk to somebody
#define SW_TMO        900   // longest walk it keeps at (15 s)

static HhSim swS;                                      // the path holder (hhPlan, hhStepAlong's data)
static u8 swMode;                                      // 0 nothing, 1 walking to furniture, 2 walking to somebody, 3 at the furniture
static u8 swKind, swTgt, swHold; static char swCh;     // the lnear value expected, the uid being visited, steps waited next to the thing
static u16 swQuiet; static short swCool, swTmo, swChatCd, swReplan; static u16 swBan;   // steps since the pad moved; pause; walk time left; chat pause; bit k = lnear kind k failed lately
static int swOx, swOy; static u8 swPi;                 // the tile the path starts from, and how far along it we are
static short swBanT;                                   // when the bans lapse

static void swStop(int cool){ swMode=0; swDrv=0; swUx=swUy=0; swCool=(short)cool; }
static int swCan(void){   // may it act now?
    if(ldead||stage<AG_CHILD||lskate||simAct||lstun>0||lcamF>0||lcN>0||tutOn||trnOn||prIn()||hzOn||exHMode()) return 0;
    if(lz>(s32)(surfH(lfx,lfy)<<8)||lvz>0) return 0;   // on the ground, not in the air or on a rail
    return 1;
}
static void swSay(const char*w){ if(lnoteT<=0){ lnote=w; lnoteT=70; } }
static void swKeys(int tx,int ty,int*ux,int*uy){   // the two pad axes that make the walk go one tile (tx,ty) the way the view is turned (feelWalk's own mapping, undone)
    for(int a=-1;a<=1;a++)for(int b=-1;b<=1;b++){ int dx=a+b, dy=b-a;
        if(cview){ int t=dx; if(cview==1){ dx=dy; dy=-t; } else if(cview==2){ dx=-dx; dy=-dy; } else { dx=-dy; dy=t; } }
        if((tx&&dx*tx>0&&!dy)||(ty&&dy*ty>0&&!dx)){ *ux=a; *uy=b; return; } }
    *ux=*uy=0;
}
static int swFloorHas(char c){ return hhHasCh(c)||(c=='C'&&hhHasCh('U')); }
static int swPlanFurn(char ch,int kind){   // a path to the furniture; 1 = there is a way
    swS.fx=lfx; swS.fy=lfy; swS.pn=swS.pi=0; swS.gok=0;
    int r=hhPlan(&swS,ch); if(r<1) return 0;
    swOx=(int)(lfx>>8); swOy=(int)(lfy>>8); swPi=0; swMode=1; swKind=(u8)kind; swCh=ch; swTmo=SW_TMO; swHold=0; return 1;
}
static int swPlanSim(int u){   // a path to somebody (and where they are now)
    int x,y; if(!uTile(u,&x,&y)) return 0;
    hhGX=x; hhGY=y; swS.fx=lfx; swS.fy=lfy; swS.pn=swS.pi=0; swS.gok=0;
    int r=hhPlan(&swS,1); if(r<1) return 0;
    swOx=(int)(lfx>>8); swOy=(int)(lfy>>8); swPi=0; swMode=2; swTgt=(u8)u; swTmo=SW_TMO; swReplan=50; swHold=0; return 1;
}
static int swWho(void){   // somebody worth a visit: friends, the glum ones to cheer up, new faces; never an enemy. -1 = nobody
    int best=-1, bs=-999, me=hhPUid, sad=moodState()==MS_SAD;
    for(int u=0;u<RU_N;u++){ if(u==me) continue; if(uGuest(u)?!guestHere(u):hhMemOf(u)<0) continue;
        int x,y; if(!uTile(u,&x,&y)) continue;
        int m=hhMemOf(u); if(m>=0&&(hhM[m].act==HA_AWAY||hhM[m].act==HA_LEAVE||hhM[m].act==HA_STAIR||hhUp[m])) continue;
        int d=relD[me][u]; if(d<-15) continue;
        int sc=d+(sad?d/2:0)+(rnd8()&31);
        if(uMood(u)<38&&uTr(me,TR_NICE)>=4) sc+=35;                   // somebody who looks down: go and cheer them up
        if(d==0&&relL[me][u]==0) sc+=20+uTr(me,TR_OUT)*4;             // a new face
        if(relF[me][u]&(RF_CRUSH|RF_LOVE|RF_STEADY)) sc+=25;
        if(sc>bs){ bs=sc; best=u; } }
    return best;
}
static int swSocPick(int a,int b){   // what your Sim says: only friendly things
    int d=relD[a][b], r=rnd8();
    if(d<-5&&socAllowed(a,b,SC_SORRY)) return SC_SORRY;
    if(uMood(b)<38&&socAllowed(a,b,SC_HUG)&&r<170) return SC_HUG;            // they look down: a hug
    if(uMood(b)<38&&socAllowed(a,b,SC_COMPL)&&r<220) return SC_COMPL;
    if(moodState()==MS_SAD&&socAllowed(a,b,SC_HUG)&&r<130) return SC_HUG;    // and you could do with one yourself
    int cpl=(relF[a][b]&(RF_STEADY|RF_LOVE))||kinRom(kin[a][b]);
    if(cpl&&socAllowed(a,b,SC_KISS)&&r<70) return SC_KISS;
    int pool[6], n=0; pool[n++]=SC_TALK;
    if(socAllowed(a,b,SC_JOKE)&&uTr(a,TR_PLAY)>=3) pool[n++]=SC_JOKE;
    if(socAllowed(a,b,SC_COMPL)&&uTr(a,TR_NICE)>=4) pool[n++]=SC_COMPL;
    if(socAllowed(a,b,SC_HIGH5)&&d>=10) pool[n++]=SC_HIGH5;
    if(socAllowed(a,b,SC_HUG)&&d>=20) pool[n++]=SC_HUG;
    return pool[(rnd8()*n)>>8];
}
// what it could do, scored; the table is need, the thing it uses, and the words it shows
typedef struct { char ch; u8 kind; u16 have; const char*say; } SwFurn;
static const SwFurn swFu[7]={ {'F',1,SR_FRIDGE,"HUNGRY"}, {'T',2,SR_TOILET,"NEED THE LOO"}, {'S',3,SR_BED,"SO TIRED"}, {'H',4,SR_SHOWER,"NEED A SHOWER"}, {'C',5,SR_SOFA,"NEED A SIT DOWN"}, {'v',11,SR_TV,"BORED  TV TIME"}, {'b',12,SR_BOOK,"BORED  A BOOK"} };
static void swPick(void){
    int hi=xo[XO_SELFWILL]==2, low=hi?45:30, ms=moodState(), sad=ms==MS_SAD, bored=ms==MS_BORED, fun=moodFunPct();
    int v[5]; v[0]=lfood; v[1]=100-lbl; v[2]=sNrg; if(simIsNight()) v[2]=v[2]>45?v[2]-45:0; v[3]=sHyg; v[4]=sCom;
    int sc[7]; for(int i=0;i<7;i++) sc[i]=0;
    for(int i=0;i<5;i++){ if(v[i]>=low) continue; sc[i]=(100-v[i])*(100-v[i])/100; }
    if(sad){   // a low mood: comfort first (a snack, the sofa), then something to take its mind off it
        if(lfood<90&&sc[0]<45) sc[0]=45; if(sCom<90&&sc[4]<50) sc[4]=50; sc[5]=sc[6]=40; if(lfood>=90&&sCom>=90){ sc[4]=0; }
    }
    if(bored||fun<low){ int u=(100-fun)*(100-fun)/100; if(sc[5]<u) sc[5]=u; if(sc[6]<u*3/4) sc[6]=u*3/4; }
    for(int i=0;i<7;i++){ if(swBan&(1<<swFu[i].kind)) sc[i]=0; if(!(simHave&swFu[i].have)&&!(i==4&&swFloorHas('U'))) sc[i]=0; if(sc[i]) sc[i]+=rnd8()&7; }
    int bi=-1, bv=0; for(int i=0;i<7;i++) if(sc[i]>bv){ bv=sc[i]; bi=i; }
    // company: lonely, glum, or just for the sake of it (HIGH more than LOW)
    int soc=0, lone=sSoc<low?(100-sSoc)*(100-sSoc)/100*(100+uTr(hhPUid,TR_OUT)*8)/100:0;
    if(hhN>0&&swChatCd<=0){ soc=lone; if(sad) soc+=30; if(!soc&&(rnd8()*100>>8)<(hi?30:12)) soc=25; if(soc&&bi>=0&&bv>=soc&&bv>=60&&(rnd8()&1)) soc=0; }   // (something urgent goes first now and then)
    if(soc>bv){ int u=swWho(); if(u>=0&&swPlanSim(u)){ swSay(sad?"FEELING LOW  NEED A CHAT":lone?"LONELY  NEED A CHAT":"GOING TO SAY HI"); return; } }
    if(bi>=0&&bv>=8){
        if(swPlanFurn(swFu[bi].ch,swFu[bi].kind)){ swSay(swFu[bi].say); return; }
        swBan|=(u16)(1<<swFu[bi].kind); swBanT=3600; }   // no way there: leave that one alone for a minute
    if(sad&&swChatCd<=0&&hhN<1){ hhSay(hhPUid,IC_SAD,"SIGH"); swChatCd=600; }   // nobody to turn to: a sigh
}
static void swWalk(void){   // one step along swS.path: fake the pad toward the next tile
    swUx=swUy=0;
    while(swPi<swS.pn){
        int tx=swOx+hhDx[swS.path[swPi]], ty=swOy+hhDy[swS.path[swPi]];   // (swOx, swOy: the last tile reached)
        s32 ex=tx*256+128-lfx, ey=ty*256+128-lfy, ax=ex<0?-ex:ex, ay=ey<0?-ey:ey;
        if(ax<56&&ay<56){ swOx=tx; swOy=ty; swPi++; continue; }   // reached this tile: on to the next
        int d=swS.path[swPi], sx=0, sy=0;   // along the path's own axis, unless a wall slide left us well off it
        if(hhDx[d]){ if(ay>110) sy=ey>0?1:-1; else sx=hhDx[d]; } else { if(ax>110) sx=ex>0?1:-1; else sy=hhDy[d]; }
        int ux, uy; swKeys(sx,sy,&ux,&uy); swUx=(s8)ux; swUy=(s8)uy; return;
    }
}
static void swChat(void){   // next to the one it came for
    int b=swTgt, x, y; swStop(SW_GAP);
    if(!uTile(b,&x,&y)) return;
    int bm=hhMemOf(b); if(bm>=0&&(hhM[bm].act==HA_SOC||hhM[bm].act==HA_USE)){ swStop(120); return; }
    int px=(int)(lfx>>8), py=(int)(lfy>>8); if((px-x)*(px-x)+(py-y)*(py-y)>4) return;   // they walked off
    swChatCd=SW_CHAT_GAP; if(bm>=0) hhM[bm].hd=(u8)((x>px?8:x<px?0:y>py?12:4));
    socDo(hhPUid,b,swSocPick(hhPUid,b));
}
static void swPre(u16*k,u16*pr){   // lifeStep, first thing: the real pad wins; with it quiet and nothing in the way the Sim carries on by itself
    if(!xo[XO_SELFWILL]){ if(swMode||swDrv) swStop(0); return; }
    if(*k&(K_UP|K_DOWN|K_LEFT|K_RIGHT|K_A|K_B|K_L|K_R|K_SEL|K_START)){ swQuiet=0; if(swMode||swDrv) swStop(xo[XO_SELFWILL]==2?240:600); return; }
    if(swQuiet<60000) swQuiet++;
    if(swChatCd>0) swChatCd--;
    if(swBanT>0&&--swBanT==0) swBan=0;
    if(!swCan()){ if(swMode||swDrv) swStop(90); return; }
    if(swCool>0){ swCool--; return; }
    if(!swMode){ swDrv=0; if(swQuiet<(xo[XO_SELFWILL]==2?SW_HI_WAIT:SW_LOW_WAIT)) return; swPick(); if(!swMode) swCool=300; else swDrv=1; return; }
    if(--swTmo<=0){ if(swMode==1) swBan|=(u16)(1<<swKind), swBanT=3600; swStop(300); return; }
    if(swMode==3){   // at the furniture: R like you would, unless somebody stands in reach (R would open their menu instead)
        swUx=swUy=0; if(lvx||lvy) return;
        if(hhNearest()>=0){ if(++swHold>180){ swBan|=(u16)(1<<swKind); swBanT=2400; swStop(SW_GAP); } return; }
        if(lnear!=swKind){ if(++swHold>40){ swBan|=(u16)(1<<swKind); swBanT=3600; swStop(SW_GAP); } return; }
        *pr|=K_R; *k|=K_R; swStop(SW_GAP); return;
    }
    if(swMode==2){
        if(--swReplan<=0){ if(!swPlanSim(swTgt)){ swStop(300); return; } }
        int x,y; if(!uTile(swTgt,&x,&y)){ swStop(300); return; }
        int px=(int)(lfx>>8), py=(int)(lfy>>8); if((px-x)*(px-x)+(py-y)*(py-y)<=2){ swDrv=0; swUx=swUy=0; if(!lvx&&!lvy) swChat(); return; }
        swDrv=1; swWalk(); return;
    }
    swDrv=1; swWalk();   // mode 1
    if(swPi>=swS.pn){ swMode=3; swHold=0; }
}
