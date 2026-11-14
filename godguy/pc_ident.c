// PC test of GodGuy step 10a, identity (source/ggid.h): every creature has a permanent id that is never reused and stays with its creature through births, deaths and compaction;
// names and personalities are worked out from the id (nothing stored); the GGN1 save trailer brings ids and flags back; an old save (no trailer) or a damaged trailer still loads, with ids handed out in list order.
#define PCTEST 1
#include <stdlib.h>
#include "source/main.c"
static int fails;
#define CHK(c,msg) do{ if(!(c)){ fails++; printf("  FAIL: %s\n",msg); } }while(0)
static u16*idSp; enum { IDSP_N=1<<22 };       // the species each id was first seen with: an id must never change creature
static void setup(unsigned seed,int size){ planetSize=size; rs=seed; makeSpecies(); makeItems(); newWorld(); chapter=3; tierOpen=4; monOn=1; }
static int inOrder(void){ for(int i=0;i<nCr;i++){ if(crId[i]==0||crId[i]>=idNext) return 0; if(i&&crId[i]<=crId[i-1]) return 0; } return 1; }
static int bound(void){ for(int i=0;i<nCr;i++){ u32 id=crId[i]; if(id>=IDSP_N) return 0; if(!idSp[id]) idSp[id]=(u16)(cr[i].sp+1); else if(idSp[id]!=cr[i].sp+1) return 0; } return 1; }
static u32 hashAll(void){ u32 h=2166136261u; const u8*p;
#define H(ptr,n) p=(const u8*)(ptr); for(size_t i=0;i<(size_t)(n);i++) h=(h^p[i])*16777619u;
    for(int y=0;y<gH;y++){ H(world[y],gW); H(bld[y],gW); H(bldv[y],gW); } H(cr,nCr*sizeof(Cr)); H(crId,nCr*sizeof(u32)); H(crFl,nCr*sizeof(u16)); H(&idNext,sizeof idNext); H(nb,nNb*sizeof(Nb)); H(&nNb,sizeof nNb); H(favCrId,sizeof favCrId);
    H(vil,sizeof vil); H(&nCr,sizeof nCr); H(&year,sizeof year); H(jr,sizeof jr); H(&jrN,sizeof jrN); return h; }
static long fsize(const char*p){ FILE*f=fopen(p,"rb"); if(!f) return -1; fseek(f,0,SEEK_END); long n=ftell(f); fclose(f); return n; }
static long trailerAt(const char*p){      // where the GGN1 trailer starts: the last "GGN1" whose creature count (8 bytes on) is nCr
    FILE*f=fopen(p,"rb"); long n=fsize(p); u8*b=(u8*)malloc((size_t)n); if(fread(b,1,(size_t)n,f)!=(size_t)n){ fclose(f); free(b); return -1; } fclose(f); long at=-1;
    for(long i=0;i+12<n;i++) if(b[i]=='G'&&b[i+1]=='G'&&b[i+2]=='N'&&b[i+3]=='1'){ u32 c; memcpy(&c,b+i+8,4); if(c==(u32)nCr) at=i; } free(b); return at; }
static void truncTo(const char*src,const char*dst,long n){ FILE*f=fopen(src,"rb"); u8*b=(u8*)malloc((size_t)n); if(fread(b,1,(size_t)n,f)!=(size_t)n) n=0; fclose(f); f=fopen(dst,"wb"); fwrite(b,1,(size_t)n,f); fclose(f); free(b); }

int main(void){
    idSp=(u16*)calloc(IDSP_N,sizeof(u16));
    // ---- 1. ids through four years of births and deaths, and through a mass die-off
    setup(4242u,128); int checks=0, bad=0; u32 maxSeen=0;
    for(int t=1;t<=4*2048;t++){ tick=t; simulate(); if((t&255)==0){ checks++; if(!inOrder()||!bound()) bad++; } }
    for(int i=0;i<nCr;i++) if(i%5!=0&&cr[i].alive) killNat(i); compact(); CHK(inOrder()&&bound(),"ids after a mass die-off"); if(nCr&&crId[nCr-1]>maxSeen) maxSeen=crId[nCr-1];
    for(int t=4*2048+1;t<=4*2048+1024;t++){ tick=t; simulate(); if((t&255)==0){ checks++; if(!inOrder()||!bound()) bad++; } }
    printf("ids: %d checks, %d bad; creatures %d, next id %u (%u ever handed out)\n",checks,bad,nCr,(unsigned)idNext,(unsigned)(idNext-1)); CHK(bad==0,"ids rising, unique and bound to their creature"); CHK(idNext>(u32)nCr+500,"births handed out new ids");
    for(int i=0;i<nCr;i++) CHK(idFind(crId[i])==i,"idFind finds every creature"); CHK(idFind(0)<0&&idFind(idNext)<0&&idFind(idNext+99)<0,"idFind refuses unused ids");
    { u32 gone=0; for(u32 id=1;id<crId[nCr-1]&&gone<2000;id++) if(idFind(id)<0) gone++; CHK(gone>100,"ids of the dead are not handed to anyone"); }
    // ---- 2. names: derived, deterministic, varied, in range
    { static char seen1[300][16]; int n1=0; static char (*full)[40]; full=(char(*)[40])malloc(3000*40); int nf=0; int lenOk=1, charOk=1, detOk=1, tongueDiff=0;
      for(u32 id=1;id<=3000;id++){ char a[40], a2[40], b[40]; idFirst(id,0,a); idFull(id,0,b); idFull(id,0,a2); if(strcmp(b,a2)) detOk=0; if(strlen(a)>13||strlen(b)>26) lenOk=0; for(char*q=b;*q;q++) if(!((*q>='A'&&*q<='Z')||*q==' ')) charOk=0;
          int dup=0; for(int k=0;k<n1&&!dup;k++) dup=!strcmp(seen1[k],a); if(!dup&&n1<300) strcpy(seen1[n1++],a); strcpy(full[nf++],b); char c2[40]; idFirst(id,2,c2); if(strcmp(a,c2)) tongueDiff++; }
      int distinct=0; qsort(full,3000,40,(int(*)(const void*,const void*))strcmp); for(int k=0;k<3000;k++) if(k==0||strcmp(full[k],full[k-1])) distinct++;
      printf("names: %d first names, %d full names in 3000 ids, %d differ by tongue\n",n1,distinct,tongueDiff); free(full);
      CHK(detOk,"a name is the same every time"); CHK(lenOk,"names fit"); CHK(charOk,"names are capital letters"); CHK(n1>=200,"216 first names per tongue are all used"); CHK(distinct>=1700,"full names are varied"); CHK(tongueDiff>2400,"another people, another name"); }
    // ---- 3. personalities
    { static int cnt[24]; int pairOk=1, rangeOk=1; for(u32 id=1;id<=24000;id++){ int o[2]; idPers(id,0,o); if(o[0]<0||o[0]>23||o[1]<0||o[1]>23) rangeOk=0; else { cnt[o[0]]++; cnt[o[1]]++; } if((o[0]>>1)==(o[1]>>1)) pairOk=0; }
      int lo=1<<30,hi=0; for(int k=0;k<24;k++){ if(cnt[k]<lo) lo=cnt[k]; if(cnt[k]>hi) hi=cnt[k]; } printf("personality: traits used %d..%d times of 24000 ids (mean 2000)\n",lo,hi); CHK(rangeOk&&pairOk,"two different pairs, in range"); CHK(lo>1500&&hi<2500,"every trait comes up about equally");
      int braveOk=1, axisOk=1, fleeOk=1, shareOk=1, warOk=1; for(u32 id=1;id<=3000;id++){ int o[2], p[2]; idPers(id,0,o); idPers(id,FL_BRAVEN,p); int hasB=0,hasT=0; for(int k=0;k<2;k++){ hasB|=p[k]==0; hasT|=p[k]==1; } if(!hasB||hasT) braveOk=0; if(!idAxis(id,FL_BRAVEN,0)) braveOk=0;
          for(int k=0;k<2;k++) if(o[k]==0&&!idAxis(id,0,0)) axisOk=0; else if(o[k]==1&&idAxis(id,0,0)) axisOk=0; else if(o[k]==4&&!idAxis(id,0,1)) axisOk=0; else if(o[k]==5&&idAxis(id,0,1)) axisOk=0; else if(o[k]==6&&!idAxis(id,0,2)) axisOk=0; else if(o[k]==7&&idAxis(id,0,2)) axisOk=0;
          int fb=persFleePct(id,0), fv=persFleePct(id,FL_VET); if(idAxis(id,0,0)){ if(fb!=10) fleeOk=0; } else if(fb!=45) fleeOk=0; if(fv>fb) fleeOk=0; int sh=persSharePct(id,0), w=persWarPct(id,0); if(sh<0||sh>95) shareOk=0; if(w<5||w>95) warOk=0; }
      CHK(braveOk,"three battles make a timid soul BRAVE (and a brave one stays brave)"); CHK(axisOk,"temperament follows the traits"); CHK(fleeOk,"the bold flee later, veterans later still"); CHK(shareOk&&warOk,"biases stay in range"); }
    // ---- 4. names fit in every message whatever the creature carries
    { int bad2=0, worst=0; for(int i=0;i<nCr&&i<600;i++){ Cr keep=cr[i]; cr[i].tr=(u8)(1+i%(TR_N-1)); cr[i].tr2=(u8)(1+(i*7)%(TR_N-1)); cr[i].rk=(u8)(i%3); cr[i].sick=(u8)(i&1); char nm[96]; memset(nm,'#',sizeof nm); crName(&cr[i],nm); int L=(int)strlen(nm); if(L>46||L==0) bad2++; if(L>worst) worst=L; cr[i]=keep; }
      printf("crName: longest %d of 46\n",worst); CHK(bad2==0,"crName never overflows"); }
    // ---- 5. save and load: ids, flags and the stars come back, and the game runs on exactly as the unsaved one
    for(int size=128;size<=192;size+=64){
        setup(777u+size,size); for(int t=1;t<=2*2048;t++){ tick=t; simulate(); } compact();
        for(int i=0;i<nCr;i+=9) crFl[i]|=FL_SCAR; crFl[3]|=FL_GRIEF; u32 id0=crId[10], idN=crId[nCr-1], nx=idNext; char name0[40]; idFull(id0,idLangOf(cr[10].sp),name0);
        u32 h0=hashAll(); int ok=saveGame("/tmp/godguy_ident.sav"); int tk=tick;
        for(int t=tk+1;t<=tk+600;t++){ tick=t; simulate(); } u32 hA=hashAll();
        planetSize=128; rs=5; newWorld(); chapter=0; memset(crId,0,sizeof crId); memset(crFl,0,sizeof crFl); idNext=77;
        int lo=loadGame("/tmp/godguy_ident.sav"); u32 h1=hashAll(); int j=idFind(id0); char n2[40]; if(j>=0) idFull(id0,idLangOf(cr[j].sp),n2); else strcpy(n2,"?");
        printf("size %d: save %d load %d | ids and flags back: %s | same name for id %u: %s | next id %u: %s\n",size,ok,lo,h0==h1?"YES":"NO",(unsigned)id0,!strcmp(name0,n2)?"YES":"NO",(unsigned)idNext,idNext==nx?"YES":"NO");
        CHK(ok&&lo,"save and load"); CHK(h0==h1,"everything, ids and flags included, comes back"); CHK(!strcmp(name0,n2),"the same id has the same name after a load"); CHK(idNext==nx&&crId[nCr-1]==idN,"the id counter comes back");
        for(int t=tk+1;t<=tk+600;t++){ tick=t; simulate(); } u32 hB=hashAll(); printf("  600 ticks later equal to the unsaved run: %s\n",hA==hB?"YES":"NO"); CHK(hA==hB,"a loaded game goes on exactly as the unsaved one"); CHK(inOrder(),"ids still rise after the run"); }
    // ---- 6. an old save (no trailer) and damaged trailers
    { setup(31u,128); for(int t=1;t<=2*2048;t++){ tick=t; simulate(); } compact(); int n=nCr; saveGame("/tmp/godguy_ident.sav"); long at=trailerAt("/tmp/godguy_ident.sav"); long sz=fsize("/tmp/godguy_ident.sav"); printf("old save: trailer at %ld of %ld bytes (%ld bytes for %d creatures)\n",at,sz,sz-at,n); CHK(at>0&&at<sz,"found the trailer");
      truncTo("/tmp/godguy_ident.sav","/tmp/godguy_old.sav",at); newWorld(); int lo=loadGameRaw("/tmp/godguy_old.sav"); int ordered=1; for(int i=0;i<nCr;i++) if(crId[i]!=(u32)i+1||crFl[i]) ordered=0;
      printf("  old save loads: %s, %d creatures, ids in list order: %s, next id %u, notables %d\n",lo?"YES":"NO",nCr,ordered?"YES":"NO",(unsigned)idNext,nNb); CHK(lo&&nCr==n&&ordered&&idNext==(u32)n+1&&nNb==0,"an old save loads with ids in list order");
      { int t0=tick; for(int t=t0+1;t<=t0+600;t++){ tick=t; simulate(); } } CHK(inOrder(),"an old save goes on with rising ids"); CHK(idNext>(u32)n+1,"and hands out new ids");
      // damage inside the trailer: half of it, a zero gap, a wrong creature count
      long cuts[3]={at+(sz-at)/2,at+14,at+9};
      for(int k=0;k<3;k++){ truncTo("/tmp/godguy_ident.sav","/tmp/godguy_bad.sav",cuts[k]); newWorld(); int l2=loadGameRaw("/tmp/godguy_bad.sav"); int ord=1; for(int i=0;i<nCr;i++) if(crId[i]!=(u32)i+1) ord=0; printf("  trailer cut %d: loads %s, ids in list order %s\n",k,l2?"YES":"NO",ord?"YES":"NO"); CHK(l2&&nCr==n&&ord,"a cut trailer falls back to list order"); }
      { FILE*f=fopen("/tmp/godguy_ident.sav","r+b"); fseek(f,at+12,SEEK_SET); fputc(0,f); fclose(f); newWorld(); int l3=loadGameRaw("/tmp/godguy_ident.sav"); int ord=1; for(int i=0;i<nCr;i++) if(crId[i]!=(u32)i+1) ord=0; printf("  zero id gap: loads %s, ids in list order %s\n",l3?"YES":"NO",ord?"YES":"NO"); CHK(l3&&ord,"a zero gap falls back to list order"); }
      { FILE*f=fopen("/tmp/godguy_ident.sav","r+b"); fseek(f,at+8,SEEK_SET); u32 w=(u32)n+1; fwrite(&w,4,1,f); fclose(f); newWorld(); int l4=loadGameRaw("/tmp/godguy_ident.sav"); int ord=1; for(int i=0;i<nCr;i++) if(crId[i]!=(u32)i+1) ord=0; printf("  wrong count: loads %s, ids in list order %s\n",l4?"YES":"NO",ord?"YES":"NO"); CHK(l4&&ord,"a wrong count falls back to list order"); } }
    printf(fails?"IDENT FAILED (%d)\n":"IDENT OK\n",fails); return fails?1:0; }
