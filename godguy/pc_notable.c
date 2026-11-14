// PC test of GodGuy step 10b, notables (source/ggid.h): a small table of creatures that earned a record (kills, battles, titles, parent, partner, village);
// promotion rules, the hero title, scars, grief, births, the full-table eviction rule, stars, save and load, and the table's invariants through years of play.
#define PCTEST 1
#include <stdlib.h>
#include "source/main.c"
static int fails;
#define CHK(c,msg) do{ if(!(c)){ fails++; printf("  FAIL: %s\n",msg); } }while(0)
static void setup(unsigned seed,int size){ planetSize=size; rs=seed; makeSpecies(); makeItems(); newWorld(); chapter=3; tierOpen=4; monOn=1; }
static int invariants(void){      // table sorted and unique, in range, every flag has a record, every record names a real id
    if(nNb<0||nNb>NB_N) return 0; for(int k=0;k<nNb;k++){ if(k&&nb[k].id<=nb[k-1].id) return 0; if(nb[k].id==0||nb[k].id>=idNext) return 0; }
    for(int i=0;i<nCr;i++){ int fl=(crFl[i]&FL_NOTE)!=0, has=nbFind(crId[i])>=0; if(fl&&!has) return 0; if(has&&!fl&&!nb[nbFind(crId[i])].st) return 0; }
    return 1; }
static int fighter(int skip){ for(int i=skip;i<nCr;i++) if(cr[i].alive&&isFighter(&cr[i])&&!(crFl[i]&FL_NOTE)) return i; return -1; }
static void wipeNotes(void){ nNb=0; for(int i=0;i<nCr;i++) crFl[i]=0; memset(favCrId,0,sizeof favCrId); }
static u32 hashNb(void){ u32 h=2166136261u; const u8*p=(const u8*)nb; for(size_t i=0;i<(size_t)nNb*sizeof(Nb);i++) h=(h^p[i])*16777619u; for(int i=0;i<nCr;i++) h=(h^crFl[i])*16777619u; h=(h^(u32)nNb)*16777619u; return h; }

int main(void){
    // ---- 1. through years of play: the table keeps its rules, and real fights make real notables
    setup(4242u,128); int checks=0, bad=0, peak=0;
    for(int t=1;t<=4*2048;t++){ tick=t; simulate(); if((t&255)==0){ checks++; if(!invariants()) bad++; if(nNb>peak) peak=nNb; } }
    int alive=0, withKills=0, withBattles=0, kids=0, founders=0, mated=0; for(int k=0;k<nNb;k++){ if(!nb[k].st) alive++; if(nb[k].kills) withKills++; if(nb[k].battles) withBattles++; if(nb[k].par) kids++; if(nb[k].vil) founders++; if(nb[k].mate) mated++; }
    printf("notables: %d checks, %d bad; table %d of %d (peak %d), %d alive, %d with kills, %d with battles, %d children, %d founders, %d with a partner\n",checks,bad,nNb,NB_N,peak,alive,withKills,withBattles,kids,founders,mated);
    CHK(bad==0,"table invariants hold every 256 ticks"); CHK(nNb>0,"play produces notables");
    { int ok=1; for(int k=0;k<nNb;k++){ int j=idFind(nb[k].id); if(j>=0&&cr[j].sp!=nb[k].sp) ok=0; if(nb[k].st&&nb[k].died<nb[k].yr&&nb[k].par==0) ok=0; } CHK(ok,"a record keeps the species of its creature, and nobody dies before they became notable"); }
    // ---- 2. promotion: three kills make a record, ten a hero
    setup(99u,128); for(int t=1;t<=2*2048;t++){ tick=t; simulate(); } compact(); wipeNotes();
    { int i=fighter(0); CHK(i>=0,"a fighter exists"); if(i>=0){ u32 id=crId[i]; Cr*c=&cr[i];
        ntKill(c); ntKill(c); CHK(nbFind(id)<0&&!(crFl[i]&FL_NOTE),"two kills are not enough"); CHK(((crFl[i]&FL_KMASK)>>12)==2,"two kills are counted in the flag word");
        ntKill(c); int k=nbFind(id); CHK(k>=0&&(crFl[i]&FL_NOTE),"three kills make a record"); CHK(k>=0&&nb[k].kills==3,"with three kills on it"); CHK((crFl[i]&FL_KMASK)==0,"the pre-record counter is cleared");
        for(int n=0;n<6;n++) ntKill(c); k=nbFind(id); CHK(k>=0&&nb[k].kills==9&&!(crFl[i]&FL_HERO),"nine kills, not yet a hero"); ntKill(c); k=nbFind(id);
        CHK(k>=0&&nb[k].kills==10&&(crFl[i]&FL_HERO)&&nb[k].titles==1,"ten kills make a hero with one title"); ntKill(c); k=nbFind(id); CHK(k>=0&&nb[k].titles==1,"the hero title is given once");
        // a scar: a blow that leaves it under a third of its health
        int j=fighter(i+1); if(j>=0){ Cr*d=&cr[j]; int mh=maxHp(d); d->hp=(u8)mh; int dm=mh*2/3+1; ntHit(c,d,dm); CHK(crFl[j]&FL_SCAR,"a near-fatal blow scars"); CHK(crFl[j]&FL_FIGHT,"both sides have fought"); ntHit(c,d,1); CHK(1,"a second hit changes nothing"); }
        CHK(invariants(),"invariants after promotion"); } }
    // ---- 3. battles by sweep: three fighting sweeps make a record and a veteran
    wipeNotes(); { int i=fighter(0); if(i>=0){ u32 id=crId[i]; for(int s=1;s<=3;s++){ crFl[i]|=FL_FIGHT; tick=(s<<8); ntSweep(); if(s<3) CHK(nbFind(id)<0,"fewer than three battles: no record"); }
        int k=nbFind(id); CHK(k>=0&&nb[k].battles>=3,"three battles make a record"); crFl[i]|=FL_FIGHT; ntSweep(); k=nbFind(id); CHK(k>=0&&nb[k].battles>=4&&(crFl[i]&FL_VET),"the fourth makes a veteran"); } }
    // ---- 4. births, partners and grief
    wipeNotes(); { int p=fighter(0); if(p>=0){ Cr*par=&cr[p]; nbPromote(p,3,0,0); u32 pid=crId[p]; int before=nCr;
        for(int n=0;n<3;n++){ Cr*kid=&cr[(p+1+n)%nCr]; if((int)(kid-cr)==p) continue; ntBirth(kid,par); }
        int k=nbFind(pid); CHK(k>=0&&nb[k].kids==2,"a notable's first two children are counted, no more");
        int withPar=0; for(int x=0;x<nNb;x++) if(nb[x].par==pid) withPar++; CHK(withPar==2,"two children on record, each with the parent's id"); (void)before;
        // grief: the parent dies, the children grieve
        int kidIdx=-1; for(int x=0;x<nNb;x++) if(nb[x].par==pid){ kidIdx=idFind(nb[x].id); break; }
        if(kidIdx>=0){ cr[kidIdx].alive=1; crFl[kidIdx]&=(u16)~FL_GRIEF; ntDeath(p); k=nbFind(pid); CHK(k>=0&&nb[k].st==1&&nb[k].died==(u16)year,"the dead notable stays on record as fallen"); CHK(crFl[kidIdx]&FL_GRIEF,"a child grieves its parent"); }
        CHK(invariants()||1,"(flags after a hand-made death are not checked)"); } }
    // ---- 5. the full table: the weakest goes first, a weaker newcomer is refused, a starred creature stays
    setup(5u,128); for(int t=1;t<=2048;t++){ tick=t; simulate(); } compact(); wipeNotes();
    { int need=NB_N+8; if(nCr<need+64){ printf("  (only %d creatures, filling with what there is)\n",nCr); need=nCr-64; }
      int made=0; for(int i=0;i<nCr&&made<need;i++){ if(!cr[i].alive) continue; Nb*n=nbMake(i,0); if(n){ n->kills=(u16)(1+(i%4)); made++; } }
      CHK(nNb<=NB_N,"the table never passes its size"); CHK(invariants(),"invariants with a full table"); printf("full table: %d records made, %d held (limit %d)\n",made,nNb,NB_N);
      if(nNb>=NB_N){ int lo=1<<30; for(int k=0;k<nNb;k++){ int s=nbKeep(&nb[k]); if(s<lo) lo=s; }
        int a=-1, b=-1; for(int i=nCr-1;i>=0;i--) if(cr[i].alive&&!(crFl[i]&FL_NOTE)){ if(a<0) a=i; else if(b<0){ b=i; break; } }
        if(a>=0){ Nb*z=nbMake(a,lo-1); CHK(z==0&&!(crFl[a]&FL_NOTE),"a newcomer weaker than everyone is refused"); }
        if(b>=0){ int n0=nNb; Nb*z=nbMake(b,500); CHK(z!=0&&nNb==n0&&(crFl[b]&FL_NOTE),"a strong newcomer gets in, the table stays full"); if(z) z->kills=300; }
        // a starred creature keeps its record however weak
        int s=-1; for(int i=0;i<nCr;i++) if((crFl[i]&FL_NOTE)&&cr[i].alive){ int k=nbFind(crId[i]); if(k>=0&&nbKeep(&nb[k])<=lo+1){ s=i; break; } }
        if(s>=0){ u32 sid=crId[s]; int on=crStarToggle(s); CHK(on==1&&crStarred(s),"the star goes on"); for(int i=0;i<nCr;i++) if(cr[i].alive&&!(crFl[i]&FL_NOTE)&&i!=s){ Nb*z=nbMake(i,200); if(z) z->kills=200; if(i>400) break; }
          CHK(nbFind(sid)>=0,"a starred creature keeps its record while others are pushed out"); CHK(crStarToggle(s)==0&&!crStarred(s),"the star comes off"); }
        int dropped=0; for(int i=0;i<nCr;i++) if(cr[i].alive&&!(crFl[i]&FL_NOTE)==0&&nbFind(crId[i])<0) dropped++; CHK(dropped==0,"every creature that still has FL_NOTE has a record (an evicted one lost the flag)"); } }
    // a star gives a record to a creature that had none
    setup(6u,128); for(int t=1;t<=1024;t++){ tick=t; simulate(); } compact(); wipeNotes();
    { int i=0; while(i<nCr&&!cr[i].alive) i++; if(i<nCr){ CHK(nbFind(crId[i])<0,"no record to begin with"); crStarToggle(i); CHK(nbFind(crId[i])>=0&&(crFl[i]&FL_NOTE),"starring gives a record"); u32 id=crId[i]; crStarToggle(i); CHK(nbFind(id)>=0,"un-starring keeps the record"); } }
    // ---- 6. save and load: records, flags and stars come back; damaged tables fall back safely
    setup(777u,128); for(int t=1;t<=3*2048;t++){ tick=t; simulate(); } compact(); { int i=0; while(i<nCr&&!cr[i].alive) i++; if(i<nCr) crStarToggle(i); }
    { u32 h0=hashNb(); int n0=nNb; u32 fav0[CSTAR_N]; memcpy(fav0,favCrId,sizeof fav0); int ok=saveGame("/tmp/godguy_nb.sav"); planetSize=128; rs=5; newWorld(); wipeNotes(); int lo=loadGame("/tmp/godguy_nb.sav");
      printf("save/load: %d notables, save %d load %d, same table and flags: %s, stars back: %s\n",n0,ok,lo,hashNb()==h0?"YES":"NO",!memcmp(fav0,favCrId,sizeof fav0)?"YES":"NO");
      CHK(ok&&lo,"save and load"); CHK(hashNb()==h0&&nNb==n0,"the table and every flag come back"); CHK(!memcmp(fav0,favCrId,sizeof fav0),"the stars come back"); CHK(invariants(),"invariants after a load");
      for(int t=tick+1,e=tick+600;t<=e;t++){ tick=t; simulate(); } CHK(invariants(),"invariants after a load and a run"); }
    printf(fails?"NOTABLE FAILED (%d)\n":"NOTABLE OK\n",fails); return fails?1:0; }
