// PC benchmark of GodGuy's big wars (the harness that comes before step 10d). It changes nothing in the game: it builds a made-up war, runs the game as it is, and records what 10d has to beat.
//   gcc -O2 -DPCTEST -w -o pc_war pc_war.c -lm
//   ./pc_war                     the benchmark: 2 armies of 6,000 (ELF against DWARF), camera far and near, 3 seeds; ends with WAR BENCH OK
//   ./pc_war N SIDE GAP TICKS [RA RB]   the same with N seeds, SIDE soldiers a side, GAP tiles between the two grids of villages, TICKS ticks (2,048 ticks are a game year), peoples RA and RB (arch 0..7)
//   ./pc_war ref FILE [N [TICKS]] write the reference, N seeds (default 3), TICKS ticks (default 2048, one game year), camera far and near. Build THIS one with -DGG_FULLTURNS: every creature then thinks at its own pace (the full per-unit run that step 10d's tiers are compared with)
//   ./pc_war cmp FILE [TOL]      run again and compare with FILE: the casualties of every checkpoint, averaged over the seeds, must be within TOL percent (default 10)
//   -DGG_FULLTURNS               benchmark build in which simulate() visits every creature every tick (see main.c); this is the reference run
//   -DWAR_NOSTAT                 build without the turn counter (the "state hash" line must be the same as the build with it: run_tests.sh compares them)
//
// What a war is here. A soldier's home is the nearest village of its own people within 14 tiles (wgSoldier), so one village's army is at most a few hundred soldiers, never thousands.
// An army of 6,000 is therefore 24 villages a side, 250 soldiers round each, every village at war with the nearest village of the other people: 48 war groups, the way the game would
// make them. They are placed on flat grass with no animals, monsters, disasters, ageing death, biomes or air; what is left to kill a soldier is the other army (and hunger).
#define PCTEST 1
#define GG_NOCIVIL 1
#define GG_NOCAPTURE 1      // villages are not taken or razed in the benchmark (see main.c)
#ifndef WAR_NOSTAT
#define GG_WARSTAT 1
#endif
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include "source/main.c"
static int fails, refFull;
#define CHK(c,msg) do{ if(!(c)){ fails++; printf("  FAIL: %s\n",msg); } }while(0)
static double now(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+t.tv_nsec*1e-9; }

#define VPS 24                    // villages a side (4 columns of 6 rows, 22 tiles apart)
#define NCP 128                   // checkpoints kept
#define CPT 128                   // ticks between checkpoints
#define YEAR 2048
static int gapTiles=20, perSide=6000, runTicks=2048, raceA=1, raceB=2;      // the two peoples at war (arch numbers 0..7)
static u32 ids0[2*MAXC/2]; static u8 side0[2*MAXC/2]; static int nIds0;

static u32 stateHash(void){ u32 h=2166136261u; const u8*p;
#define H(ptr,n) p=(const u8*)(ptr); for(size_t q=0;q<(size_t)(n);q++) h=(h^p[q])*16777619u;
    for(int y=0;y<gH;y++){ H(world[y],gW); } H(cr,nCr*sizeof(Cr)); H(crId,nCr*sizeof(u32)); H(crFl,nCr*sizeof(u16)); H(vil,sizeof vil); H(&nCr,sizeof nCr); H(&year,sizeof year); return h; }

// two armies on a flat grass world (the villages are made in turns, A B A B, so that neither people comes first in the creature list: the side whose creatures have the lower indexes gets the first blow
// and, found with this harness, loses almost nothing while the other side loses everything; see the report)
// two armies on a flat grass world: VPS villages a side, soldiers round each, every village at war with the nearest village of the other people
static void stage(unsigned seed){
    planetSize=256; rs=seed; makeSpecies(); makeItems(); newWorld(); chapter=3; tierOpen=4; monOn=0;
    gx.lAge=1; gx.lBio=1; gx.lAtmo=1; gx.dis=0; lw[LW_CIVBABY]=(u8)((LAWDEFON>>LW_CIVBABY)&1);      // CIV BABIES off: no births, so a side's losses are the war's and the crowd stays at 12,000 (at the full pace a people breeds up to the creature cap in a year)                                             // no ageing death, biomes, air, disasters or births: only the war and hunger kill
    for(int y=0;y<gH;y++) for(int x=0;x<gW;x++) world[y][x]=T_GRASS;
    nCr=0; idReset(); memset(hz,0,sizeof hz); memset(occ,0,sizeof occ); cellClear(); memset(bld,0,sizeof bld); memset(bldv,0,sizeof bldv); memset(vil,0,sizeof vil); memset(kd,0,sizeof kd); memset(vilK,0,sizeof vilK); memset(vilL,0,sizeof vilL);
    memset(boat,0,sizeof boat); memset(vilBf,0,sizeof vilBf); memset(vilSick,0,sizeof vilSick); nVilOn=0; KD_DIRTY(); tick=0; year=0; cursor=0; vilCur=0; weather=0; weatherT=900; flashT=0; atmoReset(); bioClear(); zqN=0;      // (every piece of state a run could carry over into the next is put back, so a seed always gives the same war)
    int xL=(gW-(132+gapTiles))/2, xR=xL+66+gapTiles, y0=gH/2-55; int per=perSide/VPS; nIds0=0;
    for(int k=0;k<VPS;k++) for(int side=0;side<2;side++){ int x=(side?xR:xL)+(k%4)*22, y=y0+(k/4)*22; int v=vilFound(side?raceB:raceA,x,y); if(v<0){ printf("  (no room for village %d of side %d)\n",k,side); continue; }
        int placed=0; for(int q=0;q<per;q++){ for(int tr=0;tr<200;tr++){ int dx=(int)(rnd()%21)-10, dy=(int)(rnd()%21)-10; if(dx*dx+dy*dy>100) continue; int n=addCr(side?raceB:raceA,x+dx,y+dy); if(n<0) continue; cr[n].age=20; cr[n].food=100; ids0[nIds0]=crId[n]; side0[nIds0++]=(u8)side; placed++; break; } } vil[v].pop=(u16)placed; }
    // (the spare village slots are filled with empty hamlets far from the armies: a marching soldier more than 14 tiles from home founds a colony, and a colony can join the other side's kingdom, which sets a people against itself
    // (found with this harness: the elves' own colonies, in the dwarves' kingdom, made every elf strike every other). The hamlets keep the war to the two armies)
    for(int y=8;y<gH-8;y+=14) for(int x=8;x<gW-8;x+=14){ if(y>60&&y<200) continue; if(vilFound(7,x,y)<0&&nVilOn>=VIL_N) goto filled; } filled:;
    for(int v=0;v<VIL_N;v++) if(vil[v].on){ int best=-1,bd=1<<30; for(int u=0;u<VIL_N;u++) if(vil[u].on&&vil[u].race!=vil[v].race){ int dx=vil[u].x-vil[v].x, dy=vil[u].y-vil[v].y, d=dx*dx+dy*dy; if(d<bd){ bd=d; best=u; } } if(best>=0) vil[v].foe=(u8)(best+1); }
    // (each village's pop is set to the soldiers placed round it: the kingdoms' strength is counted from it, and a war between a side with a pop and a side without would be stopped at once by the first peace talk)
    // a village's war order is cancelled every 128 ticks unless its kingdom is at war with the target's (kingdomTick), so each side is a kingdom, the two at war with spite (as the WAR power declares it)
    int kA=-1,kB=-1; for(int v=0;v<VIL_N;v++) if(vil[v].on){ int s=vil[v].race==raceB; int*k=s?&kB:&kA; if(*k<0){ *k=kdFound(v); } else vilK[v]=(u8)(*k+1); }
    if(kA>=0&&kB>=0){ kd[kB].cul=kd[kA].cul; kd[kB].rlg=kd[kA].rlg; kd[kB].rtr=kd[kA].rtr; kd[kB].lang=kd[kB].nlang=kd[kA].lang; kd[kB].clan=kd[kA].clan; kd[kB].gold=kd[kA].gold;      // (the kingdoms are made equal, so neither has a strength the other lacks: any difference in the war comes from the fighting)
        gx.war=1; kdDeclareX(kA,kB,1); } else printf("  (no kingdoms made)\n");
    warMatrix(); }
static void camera(int near){ if(near){ camX=(gW*8-256)/2; camY=(gH*8-192)/2; } else { camX=0; camY=gH*8-192; } winX=-99; }
static int survivors(int side){ int n=0; for(int k=0;k<nIds0;k++) if(side0[k]==side){ int i=idFind(ids0[k]); if(i>=0&&cr[i].alive) n++; } return n; }

typedef struct { int ncp, g0; int surv[NCP][2]; int lost[2], start[2], firstDeath, minGroups, maxGroups, maxCnt, maxPow, nEnd, nBorn, nDied; double msMean, msP99, msMax, turnsYr, visitsYr; u32 hash; long turns; } Res;
static double tt[1<<17];
static int cmpD(const void*a,const void*b){ double x=*(const double*)a, y=*(const double*)b; return x<y?-1:x>y; }
static Res run(unsigned seed,int near,int ticks){
    Res r; memset(&r,0,sizeof r); stage(seed); camera(near); wgBuild(); r.g0=wgN; r.start[0]=survivors(0); r.start[1]=survivors(1); r.firstDeath=-1; r.minGroups=1<<20;
    unsigned d0=stDied, b0=stBorn; double sumN=0, visits=0; int nSum=0; long t0=
#ifdef GG_WARSTAT
        (long)ggTurns;
#else
        0;
#endif
    if(ticks>(1<<17)) ticks=1<<17; double all=0;
    for(int t=1;t<=ticks;t++){ tick=t; unsigned dd=stDied; double a=now(); simulate(); double b=now(); tt[t-1]=b-a; all+=b-a; { int n=nCr; visits+=n>96?96+(n>2400?(n-2400)/300:0):n; } if(r.firstDeath<0&&stDied!=dd) r.firstDeath=t;
        if((t&255)==0){ int g=wgN; if(t<=2048&&g<r.minGroups) r.minGroups=g; if(g>r.maxGroups) r.maxGroups=g; for(int v=0;v<VIL_N;v++){ if(wg[v].cnt>r.maxCnt) r.maxCnt=wg[v].cnt; if(wg[v].power>r.maxPow) r.maxPow=wg[v].power; } sumN+=nCr; nSum++; }
        if(t%CPT==0&&r.ncp<NCP){ r.surv[r.ncp][0]=survivors(0); r.surv[r.ncp][1]=survivors(1); r.ncp++; } }
    r.lost[0]=r.start[0]-survivors(0); r.lost[1]=r.start[1]-survivors(1); r.nEnd=nCr; r.nBorn=(int)(stBorn-b0); r.nDied=(int)(stDied-d0); r.msMean=all*1000/ticks;
    qsort(tt,ticks,sizeof(double),cmpD); r.msP99=tt[ticks*99/100]*1000; r.msMax=tt[ticks-1]*1000;
#ifdef GG_WARSTAT
    r.turns=(long)ggTurns-t0; r.turnsYr=nSum&&sumN>0?(double)r.turns/(sumN/nSum)/((double)ticks/YEAR):0;
#endif
    r.visitsYr=nSum&&sumN>0?visits/(sumN/nSum)/((double)ticks/YEAR):0;
    r.hash=stateHash(); return r; }

static void ramReport(void){      // what the war adds in memory, and what a save costs (the DS figures are the build's own limits: 16,000 creatures, a 256 planet, 64 villages, 256 notables)
    int dsCr=16000; printf("memory: a creature costs %d bytes (Cr %d + id 4 + flags 2); at the DS cap of %d that is %d KB; war groups %d bytes (%d villages x %d); cell grid on a 256 planet %d KB; notable table at 256: %d KB\n",
        (int)sizeof(Cr)+6,(int)sizeof(Cr),dsCr,dsCr*((int)sizeof(Cr)+6)/1024,(int)sizeof wg,VIL_N,(int)sizeof(WarGrp),((256>>3)*(256>>3)*(CELL_K+2))/1024,256*(int)sizeof(Nb)/1024);
    static u8 buf[8<<20], out[9<<20]; const char*f="/tmp/gg_war.sav"; if(!saveGameRaw(f)){ printf("  (save failed)\n"); fails++; return; } FILE*h=fopen(f,"rb"); if(!h) return; long raw=(long)fread(buf,1,sizeof buf,h); fclose(h);
    unsigned pk=0; { unsigned o=0,i=0,n=(unsigned)raw,max=sizeof out-200; while(i<n){ if(o+140>max){ o=0; break; } unsigned r=1; while(i+r<n&&buf[i+r]==buf[i]&&r<129) r++; if(r>=3){ out[o++]=(u8)(126+r); out[o++]=buf[i]; i+=r; } else { unsigned st=i,l=0; while(i<n&&l<128){ unsigned rr=1; while(i+rr<n&&buf[i+rr]==buf[i]&&rr<3) rr++; if(rr>=3) break; i++; l++; } out[o++]=(u8)(l-1); for(unsigned k=0;k<l;k++) out[o++]=buf[st+k]; } } pk=o; }
    printf("save with %d creatures at the end of the run: raw %ld bytes (the DS buffer is about 700 KB), packed %u bytes\n",nCr,raw,pk); CHK(raw>0&&raw<700*1024,"the raw save of a 12,000-creature war fits the DS buffer"); CHK(pk>0&&pk<=(unsigned)raw+raw/64+200,"the packer does not blow a save up"); }

static void line(const char*tag,const Res*r){ printf("  %-10s ms/tick mean %.4f p99 %.4f max %.3f | rotation visits %.0f, thinks (turns) %.1f per creature per year | lost A %d/%d B %d/%d | first death tick %d | groups %d at the start, %d..%d later, biggest %d soldiers power %d | born %d died %d, %d left\n",
    tag,r->msMean,r->msP99,r->msMax,r->visitsYr,r->turnsYr,r->lost[0],r->start[0],r->lost[1],r->start[1],r->firstDeath,r->g0,r->minGroups,r->maxGroups,r->maxCnt,r->maxPow,r->nBorn,r->nDied,r->nEnd); }

static void curveMean(const Res*R,int n,double*a,double*b,int ncp){ for(int c=0;c<ncp;c++){ double sa=0,sb=0; for(int s=0;s<n;s++){ sa+=R[s].start[0]-R[s].surv[c][0]; sb+=R[s].start[1]-R[s].surv[c][1]; } a[c]=sa/n; b[c]=sb/n; } }

int main(int argc,char**argv){
    int nSeeds=3; const char*mode="bench"; const char*file=0; double tol=10;
    if(argc>2&&!strcmp(argv[1],"ref")){ mode="ref"; file=argv[2]; if(argc>3) nSeeds=atoi(argv[3]); if(argc>4) runTicks=atoi(argv[4]); if(nSeeds<1) nSeeds=1; }
    else if(argc>2&&!strcmp(argv[1],"cmp")){ mode="cmp"; file=argv[2]; if(argc>3) tol=atof(argv[3]); }
    else { if(argc>1) nSeeds=atoi(argv[1]); if(argc>2) perSide=atoi(argv[2]); if(argc>3) gapTiles=atoi(argv[3]); if(argc>4) runTicks=atoi(argv[4]); if(argc>6){ raceA=atoi(argv[5])&7; raceB=atoi(argv[6])&7; if(raceA==raceB) raceB=(raceA+1)&7; } if(nSeeds<1) nSeeds=1; }
    if(!strcmp(mode,"cmp")){ FILE*f=fopen(file,"r"); if(!f){ printf("cannot read %s\n",file); return 1; } int ns, fl=0, cp=0; if(fscanf(f,"GGWAR1 seeds %d side %d gap %d ticks %d races %d %d full %d cp %d",&ns,&perSide,&gapTiles,&runTicks,&raceA,&raceB,&fl,&cp)<8){ printf("bad reference file\n"); return 1; } fclose(f); nSeeds=ns; refFull=fl; }
    if(perSide<VPS) perSide=VPS; if(perSide/VPS*VPS*2>MAXC/2*2-100) perSide=(MAXC/2-100)/VPS*VPS; if(runTicks<CPT) runTicks=CPT;
    printf("GodGuy war benchmark: %d soldiers a side in %d villages (%d a village), %d tiles apart, %d ticks (%.1f game years), %d seeds; %s vs %s\n",perSide,VPS,perSide/VPS,gapTiles,runTicks,(double)runTicks/YEAR,nSeeds,arch[raceA].nm,arch[raceB].nm);
    CHK(arch[raceA].diet==4&&arch[raceB].diet==4,"the two peoples used are soldiers (diet 4)");
    static Res far_[16], near_[16]; if(nSeeds>16) nSeeds=16;
    for(int s=0;s<nSeeds;s++){ unsigned seed=1000u+s; printf("seed %u\n",seed); far_[s]=run(seed,0,runTicks); line("camera far",&far_[s]); if(s==0) ramReport(); near_[s]=run(seed,1,runTicks); line("camera near",&near_[s]); }
    int ncp=far_[0].ncp; double fa[NCP],fb[NCP],na[NCP],nb2[NCP]; curveMean(far_,nSeeds,fa,fb,ncp); curveMean(near_,nSeeds,na,nb2,ncp);
    double mf=0,mn=0,tf=0,tn=0,sf=0,sn=0; for(int s=0;s<nSeeds;s++){ mf+=far_[s].lost[0]+far_[s].lost[1]; mn+=near_[s].lost[0]+near_[s].lost[1]; tf+=far_[s].msMean; tn+=near_[s].msMean; sf+=far_[s].turnsYr; sn+=near_[s].turnsYr; } mf/=nSeeds; mn/=nSeeds; tf/=nSeeds; tn/=nSeeds; sf/=nSeeds; sn/=nSeeds;
    double vf=0; for(int s=0;s<nSeeds;s++){ double d=far_[s].lost[0]+far_[s].lost[1]-mf; vf+=d*d; } vf=nSeeds>1?sqrt(vf/(nSeeds-1)):0;
    printf("averages over %d seeds: casualties (both sides) camera far %.0f (spread between seeds %.1f%%), camera near %.0f (%+.1f%% against far); ms/tick far %.4f near %.4f; turns/creature/year far %.1f near %.1f\n",nSeeds,mf,mf>0?100*vf/mf:0,mn,mf>0?100*(mn-mf)/mf:0,tf,tn,sf,sn);
    printf("casualty curve, mean per checkpoint (every %d ticks), side A / side B, camera far:",CPT); for(int c=0;c<ncp;c++) printf(" %.0f/%.0f",fa[c],fb[c]); printf("\n");
    if(!strcmp(mode,"ref")){ FILE*f=fopen(file,"w"); if(!f){ printf("cannot write %s\n",file); return 1; } fprintf(f,"GGWAR1 seeds %d side %d gap %d ticks %d races %d %d full %d cp %d\n",nSeeds,perSide,gapTiles,runTicks,raceA,raceB,
#ifdef GG_FULLTURNS
        1,
#else
        0,
#endif
        ncp);
        for(int c=0;c<ncp;c++) fprintf(f,"far %d %.2f %.2f\n",c,fa[c],fb[c]); for(int c=0;c<ncp;c++) fprintf(f,"near %d %.2f %.2f\n",c,na[c],nb2[c]); fprintf(f,"loss far %.2f near %.2f spread %.2f\n",mf,mn,mf>0?100*vf/mf:0); fclose(f); printf("reference written to %s\n",file); }
    if(!strcmp(mode,"cmp")){ FILE*f=fopen(file,"r"); char ln[256]; int dummy; if(!fgets(ln,sizeof ln,f)){ printf("bad reference file\n"); return 1; } double worst=0; int wc=-1; char wm[8]="";
        double rfa[NCP],rfb[NCP],rna[NCP],rnb[NCP]; int nf=0,nn=0; while(fgets(ln,sizeof ln,f)){ char m[8]; int c; double a,b; if(sscanf(ln,"%7s %d %lf %lf",m,&c,&a,&b)==4&&c>=0&&c<NCP){ if(!strcmp(m,"far")){ rfa[c]=a; rfb[c]=b; nf++; } else if(!strcmp(m,"near")){ rna[c]=a; rnb[c]=b; nn++; } } } fclose(f); (void)dummy;
        CHK(nf==ncp&&nn==ncp,"the reference holds every checkpoint");
        // the deviation of a checkpoint is its error against the reference's casualties, as a share of the reference's casualties at the END of the war (early checkpoints have few casualties, so a share of those would be noise)
        double endRef=rfa[ncp-1]+rfb[ncp-1]; if(endRef<1) endRef=1; double endRefN=rna[ncp-1]+rnb[ncp-1]; if(endRefN<1) endRefN=1;
        for(int c=0;c<ncp;c++){ double d=100*fabs((fa[c]+fb[c])-(rfa[c]+rfb[c]))/endRef; if(d>worst){ worst=d; wc=c; strcpy(wm,"far"); } d=100*fabs((na[c]+nb2[c])-(rna[c]+rnb[c]))/endRefN; if(d>worst){ worst=d; wc=c; strcpy(wm,"near"); } }
        printf("compared with %s%s: the worst checkpoint (%s, #%d) is %.1f%% of the final casualties off; limit %.0f%%\n",file,refFull?" (a full per-unit run)":"",wm,wc,worst,tol);
        printf("casualties at the end, both sides: this run %.0f (camera far) / %.0f (near), reference %.0f / %.0f\n",fa[ncp-1]+fb[ncp-1],na[ncp-1]+nb2[ncp-1],rfa[ncp-1]+rfb[ncp-1],rna[ncp-1]+rnb[ncp-1]); CHK(worst<=tol,"casualties are within the tolerance of the reference run"); }
    // the hash guard: a fixed small run, printed; build with -DWAR_NOSTAT and the line must be the same (the counter changes nothing in the game)
    { Res a=run(77u,0,1024), b=run(77u,0,1024); printf("state hash %08x\n",a.hash); CHK(a.hash==b.hash,"the same seed and camera give the same game twice"); }
#ifdef GG_WARSTAT
    CHK(far_[0].turnsYr>1&&far_[0].turnsYr<1000,"the turn counter counts");
#endif
    CHK(far_[0].start[0]>=perSide/VPS*VPS*9/10&&far_[0].start[1]>=perSide/VPS*VPS*9/10,"both armies were placed");
    CHK(far_[0].g0>=2*VPS-4,"the war groups are all there as the war starts (a group a village)"); CHK(near_[0].lost[0]+near_[0].lost[1]>0,"the armies fought");
    printf(fails?"WAR BENCH FAILED (%d)\n":"WAR BENCH OK\n",fails); return fails?1:0; }
