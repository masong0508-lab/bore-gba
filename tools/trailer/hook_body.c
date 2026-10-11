volatile int tstPet[8] EWRAM_BSS; volatile int tstPetX[4] EWRAM_BSS;
static int tstPart(void){ int m=stMember(stPart); return m; }
volatile int skLog[6*600] EWRAM_BSS; volatile int skLogN;
static void skCourse(int kind){   // a flat strip with one ramp across it, the skater at its start facing +x
    int y0=MH/2, x0=3, len=26; if(x0+len>MW-2) len=MW-2-x0;
    for(int y=y0-3;y<=y0+3;y++) for(int x=x0-1;x<x0+len+1;x++){ lifeMap[y][x]='.'; }
    int rx=x0+8;
    for(int y=y0-2;y<=y0+2;y++){
        if(kind==0) lifeMap[y][rx]='4';
        else if(kind==1){ lifeMap[y][rx]='8'; lifeMap[y][rx+1]='#'; }
        else if(kind==2) lifeMap[y][rx]='<';
        else if(kind==3){ lifeMap[y][rx]='4'; lifeMap[y][rx+1]='4'; lifeMap[y][rx+2]='4'; }
        else if(kind==4){ lifeMap[y][rx]='8'; lifeMap[y][rx+1]='#'; lifeMap[y][x0-1]='6'; lifeMap[y][x0-2]='#'; }   // two quarter pipes facing each other (a halfpipe), walls behind
        else if(kind==6){ if(y==y0) for(int x=rx;x<rx+6;x++) lifeMap[y][x]='='; }   // a rail along the run
        else if(kind==5){ lifeMap[y][rx]='4'; lifeMap[y][rx+3]='2'; }   // a kicker, then a kicker facing back (landing ramp)
    }
    lfx=x0*256+128; lfy=y0*256+128; lz=lvz=0; lskate=1; lhave=1; lstun=0; lsp=0; feelReset(0); F.angF=0; lhd=0; camSnap=1; liveInvalidate(); skLogN=0;
}
static void tstRun(int c){
    if(c==30){ skCourse(tstArg); return; }
    if(c==60){ register u32 sp __asm__("sp"); volatile u8*q=(volatile u8*)0x030074c0; for(volatile u8*a=q;a<(volatile u8*)(sp-64);a++) *a=0xEE; tstPetX[0]=(int)sp; return; }
    if(c==61){ volatile u8*q=(volatile u8*)0x030074c0; int n=0; while(q[n]==0xEE&&n<4000) n++; tstPetX[1]=n; tstPetX[2]=(int)(0x03007f00-(0x030074c0+n)); return; }
    if(c==50){ look[LK_EARS]=(u8)(1+(tstArg&1)); look[LK_EARSZ]=(u8)((tstArg>>1)&7); look[LK_EARWID]=(u8)((tstArg>>4)&7); look[LK_EARSPR]=(u8)((tstArg>>7)&7); custom=0; bakeInto(spr4); return; }
    if(c==40){ exStart(XP_BASE,tstArg); exModal=0; tstPetX[0]=exP|(exM<<8)|(bdOwn<<16); return; }
    if(c==42){ int px=(int)(lfx>>8), py=(int)(lfy>>8); for(int j=-2;j<=2;j++) for(int i=-2;i<=2;i++) if((i||j)&&px+i>0&&py+j>0&&px+i<MW-1&&py+j<MH-1&&lifeMap[py+j][px+i]==46) lifeMap[py+j][px+i]=(char)(tstArg?tstArg:79); wFqView=255; wDirty=1; liveInvalidate(); tstPetX[1]=wInside[py][px]|(wInside[py][px+1]<<8)|(wInside[py+1][px+2]<<16); return; }
    if(c==41){ lfx=(tstArg&255)*256+128; lfy=((tstArg>>8)&255)*256+128; lz=lvz=0; camSnap=1; liveInvalidate(); return; }
    if(c==31){ bdOwn=(u8)tstArg; bdSave(); bdLotInit(); tstPetX[0]=bdHx|(bdHy<<8)|(lhave<<16)|(bdHere()<<20)|(bdOwn<<24); tstPetX[1]=spx|(spy<<8)|(nbT.cur<<16); return; }
    if(c==32){ if(bdHx>=0){ lfx=bdHx*256+128-200; lfy=bdHy*256+128; lz=lvz=0; camSnap=1; liveInvalidate(); } return; }
    if(c==33){ tstPetX[2]=bdOwn|(lhave<<8)|(lskate<<16); tstPetX[3]=(int)lnote; return; }
    if(c==6){ cview=0; vpFull(); mapEditor(); lifeInit(); cview=vbase; winFull(); REG_DISPCNT=0x3443; hudApplyLayout(); return; }
    if(c==8){ petN=0; static const u8 br[3]={0,13,6}; for(int i=0;i<3;i++){ petNew(&petM[i],br[(i+tstArg)%3]); petN++; petPlace(&petM[i],i); } if(tstArg>=10){ petM[0].need[PN_BLAD]=5; petM[0].potty=0; } petVramBad=1; liveInvalidate(); return; }
    if(c==10){ petAdopt(); return; }
    if(c==11){ hhSave(); petN=0; hhLoad(); tstRes[6]=petN|(petM[0].look<<8)|(petM[0].coat<<16); petStart(); return; }
    if(c==12){ petN=2; petVramBad=1; return; }
    if(c==13){ Pet*p=&petM[0]; for(int y=2;y<MH-2;y++)for(int x=2;x<MW-2;x++) if(wInside[y][x]&&hhWalk(x,y)&&hhWalk(x+1,y)&&!wInside[y][x]==0){ p->w.fx=x*256+128; p->w.fy=y*256+128; p->w.pn=p->w.pi=0; p->act=PA_IDLE; p->go=0; lfx=(x+1)*256+128; lfy=y*256+128; y=MH; break; }
        p->need[PN_BLAD]=3; p->potty=0; p->think=1; p->tr[PTR_SMART]=0; camSnap=1; liveInvalidate(); return; }
    if(c==14){ Pet*p=&petM[0]; int hold=0, leg=0, pud=0; for(int r=0;r<200;r++){ p->need[PN_BLAD]=3; p->potty=(u8)tstArg; p->w.fx=lfx+256; p->w.fy=lfy; pudN=0; lnoteT=0; int h0=sHyg; sHyg=80; petAccident(p); if(p->act==PA_HOLD) hold++; else if(pudN) pud++; else leg++; (void)h0; }
        tstRes[0]=0; tstPetX[0]=hold; tstPetX[1]=leg; tstPetX[2]=pud; return; }
    if(c==15){ Pet*p=&petM[0]; p->w.fx=lfx+150; p->w.fy=lfy; p->w.pn=p->w.pi=0; p->act=PA_SIT; p->t=2000; p->go=0; return; }
    if(c==16){ int px=(int)(lfx>>8), py=(int)(lfy>>8); for(int k=0;k<6;k++){ int x=px-2+k, y=py-2; if(x<1||y<1) continue; lifeMap[y][x]='u'; wpSet(x,y,k==0?0:k==5?WP_DEAD:k,k&1?0:3,k); } for(int i=0;i<WD_N;i++) wdSeed[i]=2; wdG[1]=5; wdG[3]=3; liveInvalidate(); wDirty=1; return; }
    if(c==17){ int px=(int)(lfx>>8), py=(int)(lfy>>8); for(int x=px-1;x<=px+1;x++) lifeMap[py-1][x]='&'; liveInvalidate(); return; }
    if(c==18){ int px=(int)(lfx>>8), py=(int)(lfy>>8); lifeMap[py-1][px-1]='u'; wpSet(px-1,py-1,WP_READY,2,1); liveInvalidate(); return; }
    if(c==19){ wdRoll(); return; }
    if(c==20){ musStop(); ambG=0; look[LK_VOICE]=(u8)tstArg; voxPlay(tstArg==9?V_disagree:V_joke_good); tstPetX[0]=voxPitch(hhPUid); tstPetX[1]=(int)sfxStp; tstPetX[2]=look[LK_SEX]|(stage<<8); return; }
    if(c==21){ musStop(); mPlay=0; return; }
    if(c==22){ nbGenS(tstArg&3,"HUGE TOWN",24+((tstArg>>2)&3)*8); nbTS=-1; nbSave(); nbOk=1; neighborhoodScreen(); return; }
    if(c==23){ nbGenS(1,"RT TOWN",40); nbTS=-1; int e=nbSave(); slScan(); static Town t2 EWRAM_BSS; int ok=nbRead(nbTS,&t2); int same=1; const u8*a=(const u8*)&nbT,*b=(const u8*)&t2; for(unsigned i=4;i<sizeof(Town);i++) if(a[i]!=b[i]){ same=0; tstPetX[3]=(int)i; break; }
        int lots=0; for(int i=0;i<NB_LOTS;i++) lots+=nbT.lot[i].on; tstPetX[0]=e; tstPetX[1]=ok|(same<<1)|(slI[nbTS].span<<4); tstPetX[2]=lots|(nbT.w<<8); return; }
    if(c==24){ if(!nbOk){ nbGenS(0,"TEST TOWN",32); nbT.cur=0; nbT.NB_ACT=1; nbTS=-1; nbSave(); nbOk=1; } int fi=-1,nl=0,liv=0,fr4=0; for(int k=0;k<NB_LOTS;k++){ if(!nbT.lot[k].on&&fi<0) fi=k; if(nbT.lot[k].on){ nl++; if(nbLives(k)) liv++; } } for(int y=0;y+4<=nbT.h;y++) for(int x=0;x+4<=nbT.w;x++){ int ok=1; for(int yy=y;yy<y+4&&ok;yy++) for(int xx=x;xx<x+4;xx++) if(nbAt(xx,yy)>=0||NB_GR(nbT.cell[yy][xx])==NT_WATER){ ok=0; break; } fr4+=ok; } tstPetX[1]=fi|(nl<<8)|(liv<<16); tstPetX[2]=fr4|(nbT.w<<16); int d=trvPlace(CT_TACO+tstArg); tstPetX[0]=d; if(d>=0) trvGo(d); return; }
    if(c==25){ for(int y=1;y<MH-1;y++) for(int x=1;x<MW-1;x++) if(lifeMap[y][x]=='@'&&lifeMap[y+1][x]=='.'){ lfx=x*256+128; lfy=(y+1)*256+128; lz=lvz=0; camSnap=1; liveInvalidate(); lchill=2000; y=MH; break; } return; }
    if(c==9){ for(int i=0;i<petN;i++) petM[i].think=1; return; }
    if(c==7){ if(flGo(1)){ for(int y=1;y<MH-1;y++)for(int x=1;x<MW-1;x++) if(floorMap[y][x]!=FL_AIR&&lifeMap[y][x]=='.'&&floorMap[y-1][x]!=FL_AIR&&floorMap[y+1][x]!=FL_AIR&&floorMap[y][x-1]!=FL_AIR&&floorMap[y][x+1]!=FL_AIR){ lfx=x*256+128; lfy=y*256+128; lz=lvz=0; tstRes[7]=x|(y<<8); y=MH; break; } liveInvalidate(); camSnap=1; } return; }
    int m=tstPart(); if(m<0) return; int a=hhPUid, b=hhM[m].uid;
    switch(c){
    case 1: simMin=19*60; relD[a][b]=relD[b][a]=80; relL[a][b]=relL[b][a]=80; relF[a][b]|=RF_KISSED|RF_CRUSH|RF_LOVE|RF_STEADY; relF[b][a]|=RF_KISSED|RF_CRUSH|RF_LOVE|RF_STEADY; hhM[m].need[HN_FUN]=30; hhM[m].need[HN_REST]=40; break;
    case 2: socDo(a,b,SC_TV); break;
    case 3: simMin=23*60; socDo(a,b,SC_BED); break;
    case 4: { int px=(int)(hhM[m].fx>>8), py=(int)(hhM[m].fy>>8); for(int d=0;d<8;d++){ static const s8 ox[8]={1,-1,0,0,1,-1,1,-1}, oy[8]={0,0,1,-1,1,1,-1,-1}; int x=px+ox[d], y=py+oy[d]; if(hhWalk(x,y)){ lfx=x*256+128; lfy=y*256+128; break; } }
              simAct=tstArg; simActT=900; } break;
    case 5: relD[a][b]=relD[b][a]=40; break;
    case 6: cview=0; vpFull(); mapEditor(); lifeInit(); cview=vbase; winFull(); REG_DISPCNT=0x3443; hudApplyLayout(); break;
    }
    liveInvalidate();
}
static void tstPeek(void){ if(skLogN<600){ volatile int*L=&skLog[6*skLogN++]; L[0]=(int)lz; L[1]=surfH(lfx,lfy); L[2]=F.spd; L[3]=lvz; L[4]=(int)lfx; L[5]=lstun|(lskate<<8)|(lgrind<<9)|(lqp<<10); }
    if(petN){ const Pet*p=&petM[0]; tstPet[0]=p->act|(p->go<<8)|(p->need[PN_BLAD]<<16)|(p->pose<<24); tstPet[1]=p->think|(p->t<<16); tstPet[2]=(int)(p->w.fx>>8)|((int)(p->w.fy>>8)<<8)|(wInside[p->w.fy>>8][p->w.fx>>8]<<16); tstPet[3]=pudN|(petN<<8); tstPet[4]=lnoteT; tstPet[5]=petAtHome(); } int m=tstPart(); if(m<0){ tstRes[0]=-1; return; } const HhSim*s=&hhM[m]; int a=hhPUid,b=s->uid;
    tstRes[0]=s->act|(s->use<<8)|(s->jl<<16)|(s->jt<<24); tstRes[1]=(int)(s->fx>>8)|((int)(s->fy>>8)<<8)|((int)(lfx>>8)<<16)|((int)(lfy>>8)<<24);
    tstRes[2]=relD[a][b]; tstRes[3]=relL[a][b]; tstRes[4]=lnoteT; tstRes[5]=simAct|(simMin<<8); tstRes[6]=(int)lnote; tstRes[7]=s->item; }
