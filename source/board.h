// board.h - YOUR SKATEBOARD: you own one or you do not. Nobody hands you a board on every lot any more (the old pickup beside the arrival).
//   BUY ONE   at the SKATE SHOP (venues.h): A COMPLETE BOARD, or A PRO DECK (a little faster, a little more pop).
//   FIND ONE  every PARK, SKATE PARK, PLAZA and OLD TOWN hides a board in its most out-of-the-way spot (the floor tile you can reach that is
//             farthest from where you arrive). Nothing marks it until you are within two tiles: then it glints. Walk over it and it is yours.
//   WHERE     only where a board belongs: home and other house lots, parks, skate parks, plazas and old town. Not in shops, restaurants, the gym,
//             the bank, the hospital, the cinema, the casino, the lounge, the dispensary, the arms shop or the prison: L says NO SKATING IN HERE.
// SAVED  BD_OFF ('B' 'D', what you own, checksum), and in the player save (chunk 'O'). A life saved before boards were owned keeps one (it had one).
// Needs before it: venues.h (CT_), neighborhood.h, prison.h (prIn), wpScr (weapons.h), hhWalk, the life globals.
#define BD_OFF 7552
_Static_assert(BD_OFF+4<=LG_OFF,"the board block sits before the life stats");   // (expand.h checks the handheld's block ends before it)
static u8 bdSum(volatile const u8*m){ return (u8)(0x42+m[2]*7); }
static void bdSave(void){ volatile u8*m=SRAM_BASE+BD_OFF; m[0]='B'; m[1]='D'; m[2]=bdOwn; m[3]=bdSum(m); }
static int bdLoadFrom(volatile const u8*m){ if(m[0]!='B'||m[1]!='D'||m[2]>2||m[3]!=bdSum(m)) return 0; bdOwn=m[2]; return 1; }
static void bdLoad(void){ if(!bdLoadFrom(SRAM_BASE+BD_OFF)) bdOwn=1; }   // (no block: a life from before owned boards: it had one)
static int bdHere(void){   // may you ride a board on this lot?
    if(prIn()) return 0;
    if(!nbOk||nbT.cur>=NB_LOTS) return 1;
    const NbLot*L=&nbT.lot[nbT.cur]; if(!L->on||L->kind==LKIND_RES) return 1;
    return L->type==CT_PARK||L->type==CT_SKATE||L->type==CT_BOTH||L->type==CT_PLAZA||L->type==CT_OLDTOWN;
}
static int bdHides(void){ if(!nbOk||nbT.cur>=NB_LOTS) return 0; const NbLot*L=&nbT.lot[nbT.cur]; return L->on&&L->kind==LKIND_COMM&&bdHere(); }   // this lot hides a board
static s8 bdHx=-1, bdHy=-1;   // the hidden board on this lot (-1: none)
static void bdLotInit(void){   // main.c lifeInit: the old free pickup goes, the hidden board is worked out, and whether you ride here
    if(nbOk&&bdx>=0){ if(lifeMap[bdy][bdx]=='B') lifeMap[bdy][bdx]='.'; bdx=bdy=-1; }   // (towns: no board lying by the door)
    bdHx=bdHy=-1;
    if(!bdOwn&&bdHides()&&spx>=0&&!curFl){   // the floor tile farthest from the arrival you can walk to (a breadth-first search over the lot)
        u16*q=bfsQ; u8 (*seen)[MW]=(u8(*)[MW])(void*)hhDist;   // (the shared search queue and marks)
        for(int y=0;y<MH;y++) for(int x=0;x<MW;x++) seen[y][x]=0;
        int h=0, t=0; q[t++]=(u16)(spy*MW+spx); seen[spy][spx]=1; int last=-1;
        while(h<t){ int c=q[h++], cx=c%MW, cy=c/MW; if(lifeMap[cy][cx]=='.'&&(cx!=spx||cy!=spy)) last=c;
            static const s8 dx[4]={1,-1,0,0}, dy[4]={0,0,1,-1};
            for(int d=0;d<4;d++){ int nx=cx+dx[d], ny=cy+dy[d]; if(nx<1||ny<1||nx>=MW-1||ny>=MH-1||seen[ny][nx]||!hhWalk(nx,ny)) continue; seen[ny][nx]=1; q[t++]=(u16)(ny*MW+nx); } }
        if(last>=0){ bdHx=(s8)(last%MW); bdHy=(s8)(last/MW); } }
    lhave=bdOwn&&bdHere(); if(!lhave) lskate=0;
}
static void bdGot(int lvl,const char*msg){ if(bdOwn<lvl) bdOwn=(u8)lvl; bdSave(); sgDirty=1; lhave=bdHere(); lnote=msg; lnoteT=150; moodEvent(M_GOT_BOARD); voxPlay(V_yahoo); }
static void bdTick(void){   // main.c lifeStep: walking over the hidden board
    if(bdHx<0||bdOwn||lz>(8<<8)) return;
    if((int)(lfx>>8)==bdHx&&(int)(lfy>>8)==bdHy){ bdHx=bdHy=-1; bdGot(1,"YOU FOUND A SKATEBOARD  IT IS YOURS"); sfxPlay(SFX_STICK); }
}
static void bdDraw(void){   // the glint, only when you are close (weapons.h wpDraw)
    if(bdHx<0||bdOwn) return;
    int dx=(int)(lfx>>8)-bdHx, dy=(int)(lfy>>8)-bdHy; if(dx*dx+dy*dy>4) return;
    int sx,sy; wpScr(bdHx*256+128,bdHy*256+128,1,&sx,&sy); int t=(int)((lfr>>3)&3);
    rect(sx-4,sy-1,9,2,RGB(18,10,4)); px(sx-3,sy+1,RGB(6,6,6)); px(sx+3,sy+1,RGB(6,6,6));   // a board lying there, its wheels
    if(t<2){ u16 c=t?RGB(31,31,31):RGB(31,28,10); px(sx+2,sy-4,c); px(sx+1,sy-4,c); px(sx+3,sy-4,c); px(sx+2,sy-5,c); px(sx+2,sy-3,c); }
}
static void sgEncBoard(SlW*w){ bdSave(); volatile u8*m=SRAM_BASE+BD_OFF; for(int i=0;i<4;i++) slwPut(w,m[i]); }
