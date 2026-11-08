// PC test of GodGuy's story, monsters and unlocks: a simple "god bot" plays (settles peoples, gifts weapons, smites the worst monsters, blesses villages) and the log shows
// the chapters falling one by one, the bestiary filling and the monsters keeping the world dangerous without ending it.
//   gcc -O2 -DPCTEST -o pc_story pc_story.c -lm && ./pc_story <seed> <years> <size 128|192|256> [bot 0|1]
#define PCTEST 1
#include <stdlib.h>
#include "source/main.c"
static int findPw(int kind){ for(int i=0;i<PW_N;i++) if(pw[i].kind==kind) return i; return 0; }
static int bestItem(int kind){ int b=-1; for(int i=0;i<nIt;i++) if(itOpen(i)&&it[i].kind==kind&&(b<0||it[i].val>=it[b].val)) b=i; return b; }
int main(int argc,char**argv){
    unsigned seed=argc>1?atoi(argv[1]):7; int years=argc>2?atoi(argv[2]):12; planetSize=argc>3?atoi(argv[3]):128; int bot=argc>4?atoi(argv[4]):1;
    rs=seed*2654435761u+1; makeSpecies(); makeItems(); newWorld(); storyOn=1; int lastCh=-1, lastSeen=0; long smites=0;
    printf("planet %d x %d, %d species, %d items\n",WW,WH,nSp,nIt);
    for(int t=1;t<=years*2048;t++){ tick=t; simulate();
        if(bot&&t%90==0){ brush=1;
            // 1. keep several peoples alive: found a village for a missing people
            if(nVilOn<6&&(rnd()&3)==0){ int r=(int)(rnd()%5); power=findPw(PK_SETTLE); spSel=r; usePower(rnd()%WW,rnd()%WH,1); }
            // 2. help the folk that are in trouble: feed and bless
            for(int v=0;v<VIL_N;v++) if(vil[v].on&&vil[v].stock<60){ power=findPw(PK_BLESS); usePower(vil[v].x,vil[v].y,1); }
            // 3. gifts: the best open weapon and armour to people near the front
            if((t/90)%4==0){ int w=bestItem(IK_WEAPON), a=bestItem(IK_ARMOR); for(int k=0;k<6;k++){ int i=rnd()%nCr; if(cr[i].alive&&arch[sp[cr[i].sp].a].diet==4){ power=findPw(PK_GIFT); itSel=(k&1)?a:w; if(itSel>=0) usePower(cr[i].x,cr[i].y,1); } } }
            // 4. smite the strongest monster that threatens a village (once in a while)
            if((t/90)%3==0){ int best=-1,bh=0; for(int i=0;i<nCr;i++) if(cr[i].alive&&arch[sp[cr[i].sp].a].diet==5&&vilAny(cr[i].x,cr[i].y,14)>=0&&cr[i].hp>bh){ bh=cr[i].hp; best=i; }
                if(best>=0){ power=findPw(PK_KILL); usePower(cr[best].x,cr[best].y,1); smites++; } }
            // 5. repopulate: more people in a village that is shrinking
            if((t/90)%5==0){ for(int v=0;v<VIL_N;v++) if(vil[v].on&&vil[v].pop<8){ power=findPw(PK_SPAWN); spSel=vil[v].race; brush=3; usePower(vil[v].x+1,vil[v].y,1); brush=1; break; } }
        }
        if(chapter!=lastCh||t%2048==0){ int hh; int pp=vilTotals(&hh); int mon=monCount(); if(t%2048==0||chapter!=lastCh) printf("y%02d t=%6d ch=%2d/%d tier=%2d | alive %4d folk %3d homes %3d vill %2d | monsters %2d seen %3d slain %4u smites %ld | %s\n",year,t,chapter,CHAP_N,tierOpen,nCr,pp,hh,nVilOn,mon,seenN,(unsigned)stKill,smites,info); lastCh=chapter; }
    }
    int open; int tot=itCount(&open); printf("END: chapter %d/%d done=%d, items open %d/%d, bestiary %d/320, slain %u, used %u, gifts %u\n",chapter,CHAP_N,storyDone,open,tot,seenN,(unsigned)stKill,(unsigned)stUse,(unsigned)stGift);
    return 0; }
