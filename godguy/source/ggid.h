// ggid.h -- GodGuy step 10a + 10b: who a creature IS.
//
// 10a identity. Every creature has a permanent id (crId, never reused; ids rise along the creature list, so an id is found by binary search) and a 16-bit flag word (crFl).
//     The name and the personality are NOT stored: they are worked out from the id (and the creature's people), so the same id is the same person after a save, a load or a move
//     between builds. Only what happened to a creature is stored (the flag word); experience can overrule the personality (a timid soul who lives through three battles is BRAVE).
// 10b notables. A small table (nb[], 32 bytes a record, sorted by id) holds the creatures that earned a record: kills, battles survived, titles, parent / partner / lord, village founded.
//     A creature is promoted to it when it has killed three, lived through three battles, risen to a champion's level, become an elite or a boss, founded a village, been
//     starred in the World Browser, or been born to a notable. When the table is full the lowest-scoring entry is dropped.
//
// Nothing in here calls rnd() and nothing here changes a creature's own fields, so the simulation of every creature is exactly what it was before this step; only the records move.
// (The personality biases at the bottom are plain functions of the id for the next steps to use; they are not wired into the thinking yet.)
#ifndef GGID_H
#define GGID_H

#define FL_VET     0x0001      // lived through three or more battles
#define FL_SCAR    0x0002      // was wounded to under a third of its health in a fight
#define FL_GRIEF   0x0004      // a parent, partner or child died (fades after a few sweeps)
#define FL_HERO    0x0008      // ten kills
#define FL_DESERT  0x0010      // (reserved for the war groups of step 10c)
#define FL_RULER   0x0020      // (reserved)
#define FL_FOUND   0x0040      // founded a village
#define FL_NOTE    0x0080      // has a record in the notable table
#define FL_FIGHT   0x0100      // fought since the last sweep
#define FL_BRAVEN  0x0200      // experience made it BRAVE
#define FL_KMASK   0x3000      // bits 12..13: kills counted before the record exists (0..3)
#define FL_BMASK   0xC000      // bits 14..15: battles counted before the record exists (0..3)

#if defined(GG_DSI16)
#define NB_N 1024              // notables the table holds: 256 on the DS, 1,024 in DSi mode, 4,096 on the 3DS and PC
#elif defined(GG_BIGMAP)
#define NB_N 4096
#else
#define NB_N 256
#endif
#define CSTAR_N 16             // creatures starred in the World Browser

typedef struct { u32 id, par, mate, lord; u16 sp, yr, died, kills; u8 battles, wounds, titles, kids, vil, st; u16 pad; } Nb;   // 32 bytes. yr: the year it was born (when born to a notable) or became notable. st: 0 alive, 1 dead or gone. vil: the village it founded, +1
typedef char nbSizeCheck[sizeof(Nb)==32?1:-1];

// crId / crFl / idNext (6 bytes a creature) are declared in main.c beside cr[] and kept in step with it by addCr, compact and the loader.
static Nb nb[NB_N]; static int nNb; static u32 favCrId[CSTAR_N];

static void idReset(void){ idNext=1; nNb=0; memset(favCrId,0,sizeof favCrId); }

// ---------------------------------------------------------------------------------------------- names and personalities, worked out from the id
static u32 hmix(u32 x){ x^=x>>16; x*=0x7feb352dU; x^=x>>15; x*=0x846ca68bU; x^=x>>16; return x; }
static const char* const epiN[16]={"THE BOLD","THE GREY","THE RED","THE FAIR","THE LAME","THE TALL","THE SMALL","THE QUICK","THE STERN","THE MERRY","THE DARK","THE PALE","THE LUCKY","THE BLACK","THE GOLDEN","THE LOUD"};
static const char* const persN[24]={"BRAVE","TIMID","GREEDY","GENEROUS","KIND","CRUEL","LOYAL","FICKLE","WISE","FOOLISH","PROUD","HUMBLE","CALM","ANGRY","HONEST","SLY","LAZY","DILIGENT","CURIOUS","DULL","PIOUS","PROFANE","JOLLY","GLOOMY"};   // twelve pairs of opposites: trait t and t^1
static int idLangOf(int spIdx){ return raceLang(sp[spIdx].a&7); }
static void idFirst(u32 id,int l,char*o){ u32 h=hmix(id*0x9E3779B1u+0x1234567u); strcpy(o,pn1[l][h%6]); strcat(o,pn2[l][(h>>8)%6]); strcat(o,pn3[l][(h>>16)%6]); }      // 216 first names a tongue
static void idFull(u32 id,int l,char*o){ idFirst(id,l,o); strcat(o," "); strcat(o,epiN[(hmix(id*0x9E3779B1u+0x1234567u)>>24)&15]); }                                      // 3,456 with the epithet
static void idPers(u32 id,u16 fl,int out[2]){
    u32 h=hmix(id*0x85EBCA6Bu+77u); int p0=(int)(h%12), p1=(p0+1+(int)((h>>8)%11))%12;   // two different pairs, one side of each
    out[0]=p0*2+(int)((h>>16)&1); out[1]=p1*2+(int)((h>>17)&1);
    if(fl&FL_BRAVEN){ int has=0; for(int k=0;k<2;k++){ if(out[k]==1) out[k]=0; if(out[k]==0) has=1; } if(!has) out[0]=0; } }
static int idAxis(u32 id,u16 fl,int axis){     // temperament: 0 bold (1) or timid (0), 1 kind or cruel, 2 loyal or fickle. The two traits decide it when they speak to it, the id otherwise
    static const u8 yes[3]={0,4,6};                // BRAVE, KIND, LOYAL (their opposites are yes+1)
    int o[2]; idPers(id,fl,o); for(int k=0;k<2;k++){ if(o[k]==yes[axis]) return 1; if(o[k]==yes[axis]+1) return 0; }
    return (int)((hmix(id*0xC2B2AE35u+(u32)axis)>>9)&1); }
// (the three biases below are for the next steps: how likely it is to run, to join a war, to share food. Percentages, no division, no floats.)
static int persFleePct(u32 id,u16 fl){ int p=25+(idAxis(id,fl,0)?-15:20); if(fl&FL_VET) p-=5; return p<5?5:p; }                 // flees under this share of its health
static int persWarPct(u32 id,u16 fl){ int p=50+(idAxis(id,fl,0)?20:-20)+(idAxis(id,fl,2)?15:-15)+(idAxis(id,fl,1)?-10:10); return p<5?5:p>95?95:p; }
static int persSharePct(u32 id,u16 fl){ int o[2]; idPers(id,fl,o); int p=30+(idAxis(id,fl,1)?15:-15); for(int k=0;k<2;k++){ if(o[k]==3) p+=30; if(o[k]==2) p-=20; } return p<0?0:p>95?95:p; }

static int crNamed(const Cr*c){ return arch[sp[c->sp].a].diet==4||c->rk||(crFl[c-cr]&FL_NOTE); }   // people, ranked monsters and anyone with a record have a name; animals stay "WOLF"
static void crFirst(const Cr*c,char*o){ idFirst(crId[c-cr],idLangOf(c->sp),o); }

// ---------------------------------------------------------------------------------------------- finding creatures by id
static int idFind(u32 id){ int lo=0,hi=nCr-1; while(lo<=hi){ int m=(lo+hi)>>1; u32 v=crId[m]; if(v==id) return m; if(v<id) lo=m+1; else hi=m-1; } return -1; }
static void idAssign(void){ idNext=1; for(int i=0;i<nCr;i++){ crId[i]=idNext++; crFl[i]=0; } nNb=0; memset(favCrId,0,sizeof favCrId); }   // an old save (or a bad trailer): ids in list order

// ---------------------------------------------------------------------------------------------- the notable table
static int nbPos(u32 id){ int lo=0,hi=nNb; while(lo<hi){ int m=(lo+hi)>>1; if(nb[m].id<id) lo=m+1; else hi=m; } return lo; }
static int nbFind(u32 id){ int p=nbPos(id); return p<nNb&&nb[p].id==id?p:-1; }
static int idStarred(u32 id){ if(!id) return 0; for(int k=0;k<CSTAR_N;k++) if(favCrId[k]==id) return 1; return 0; }
static int nbScore(const Nb*n){ int s=n->kills*2+n->battles*3+n->titles*5+n->kids+n->wounds; if(n->vil) s+=5; if(idStarred(n->id)) s+=50; return s>32000?32000:s; }
static int nbKeep(const Nb*n){ int s=nbScore(n); return n->st?s/2:s; }       // the fallen count for half: they go first
static void nbLog(const char*what,const Nb*n){ char nm[24], b[48]; idFirst(n->id,idLangOf(n->sp),nm); siprintf(b,"%.12s %s",nm,what); logEv(b); }
static void nbRemove(int k){ memmove(&nb[k],&nb[k+1],(size_t)(nNb-k-1)*sizeof(Nb)); nNb--; }
// A record for creature ci (made if it has none). Returns 0 when the table is full and everything in it ranks above this newcomer's first score sc0. The pointer is good only until the next table change.
static Nb* nbMake(int ci,int sc0){
    u32 id=crId[ci]; int p=nbPos(id); if(p<nNb&&nb[p].id==id) return &nb[p];
    if(nNb>=NB_N){ int v=0, vk=nbKeep(&nb[0]); for(int k=1;k<nNb;k++){ int s=nbKeep(&nb[k]); if(s<vk){ vk=s; v=k; } } if(sc0<vk) return 0;
        if(!nb[v].st){ int j=idFind(nb[v].id); if(j>=0) crFl[j]&=(u16)~FL_NOTE; if(nbScore(&nb[v])>=6) nbLog("FADES FROM FAME",&nb[v]); }      // an entry that is dropped while alive still goes into the journal if it mattered
        nbRemove(v); p=nbPos(id); }
    memmove(&nb[p+1],&nb[p],(size_t)(nNb-p)*sizeof(Nb)); nNb++; Nb*n=&nb[p]; memset(n,0,sizeof *n); n->id=id; n->sp=cr[ci].sp; n->yr=(u16)year; crFl[ci]|=FL_NOTE; return n; }
static int nbPromote(int ci,int kills,int battles,int titles){
    Nb*n=nbMake(ci,kills*2+battles*3+titles*5); if(!n) return 0; if(n->kills<kills) n->kills=(u16)kills; if(n->battles<battles) n->battles=(u8)battles; if(n->titles<titles) n->titles=(u8)titles;
    crFl[ci]&=(u16)~(FL_KMASK|FL_BMASK); return 1; }

static int isFighter(const Cr*c){ return arch[sp[c->sp].a].diet==4||c->rk; }
static void ntKill(Cr*c){      // called by gainXp: c has just killed something
    if(!isFighter(c)) return; int i=(int)(c-cr); u16 f=crFl[i]; crFl[i]|=FL_FIGHT;
    if(f&FL_NOTE){ int k=nbFind(crId[i]); if(k<0) return; Nb*n=&nb[k]; if(n->kills<65535) n->kills++;
        if(n->kills>=10&&!(f&FL_HERO)){ crFl[i]|=FL_HERO; n->titles++; nbLog("BECOMES A HERO",n); } }
    else { int kc=(f&FL_KMASK)>>12; if(kc<3){ kc++; crFl[i]=(u16)((crFl[i]&~FL_KMASK)|(kc<<12)); } if(kc>=3) nbPromote(i,3,(f&FL_BMASK)>>14,0); } }
static void ntHit(Cr*a,Cr*d,int dm){      // a strikes d for dm (before the blow lands): both have fought; a blow that leaves d under a third of its health scars it
    int ai=(int)(a-cr), di=(int)(d-cr); if(isFighter(a)) crFl[ai]|=FL_FIGHT; if(!isFighter(d)) return; crFl[di]|=FL_FIGHT;
    if(d->hp>dm&&d->hp-dm<maxHp(d)/3&&!(crFl[di]&FL_SCAR)){ crFl[di]|=FL_SCAR; if(crFl[di]&FL_NOTE){ int k=nbFind(crId[di]); if(k>=0&&nb[k].wounds<255) nb[k].wounds++; } } }
static void ntGrieve(u32 id){ int j=idFind(id); if(j>=0&&cr[j].alive) crFl[j]|=FL_GRIEF; }
static void ntGone(u32 id){ int k=nbFind(id); if(k>=0&&!nb[k].st){ nb[k].st=1; nb[k].died=(u16)year; } }   // (compact: a creature that left the list without being killed, a settler, say)
static void ntDeath(int i){      // called by killCr before the creature is marked dead
    u16 f=crFl[i]; if(!(f&FL_NOTE)) return; int k=nbFind(crId[i]); if(k<0) return; Nb*n=&nb[k]; n->st=1; n->died=(u16)year; u32 id=n->id, par=n->par, mate=n->mate;
    if((f&FL_HERO)||n->kills>=5){ if(natural) nbLog("DIES OF AGE",n); else if(!cr[i].rk) nbLog("FALLS IN BATTLE",n); }      // (elites and bosses already have their own FALLS line)
    if(par) ntGrieve(par); if(mate) ntGrieve(mate); for(int x=0;x<nNb;x++) if(nb[x].id!=id&&(nb[x].par==id||nb[x].mate==id)) ntGrieve(nb[x].id); }
static void ntBirth(Cr*kid,const Cr*par){      // a notable's first two children are written down as notable too (their parent is on record); the parent's partner is whoever stands beside it
    int pi=(int)(par-cr), ki=(int)(kid-cr); if(!(crFl[pi]&FL_NOTE)) return; int k=nbFind(crId[pi]); if(k<0) return; Nb*n=&nb[k]; if(n->kids>=2) return; n->kids++; u32 pid=n->id;
    if(!n->mate) for(int d=0;d<4;d++){ int x=par->x+dxs[d], y=par->y+dys[d]; if(x<0||y<0||x>=WW||y>=WH||!occ[y][x]) continue; int j=occ[y][x]-1; if(j!=pi&&j!=ki&&cr[j].alive&&cr[j].sp==par->sp){ n->mate=crId[j]; break; } }
    Nb*m=nbMake(ki,2); if(m){ m->par=pid; m->yr=(u16)year; } }
static void ntFounder(Cr*c,int v){
    int i=(int)(c-cr); crFl[i]|=FL_FOUND; Nb*n=nbMake(i,5); if(n){ n->titles++; n->vil=(u8)(v+1); }
    { char nm[24], vn[24], b[48]; crFirst(c,nm); vilName(v,vn); siprintf(b,"%.10s FOUNDS %.12s",nm,vn); logEv(b); } }
static void ntSweep(void){      // every 256 ticks, right after compact(): the slow bookkeeping. Cost: one pass over the creature list.
    int fade=(((tick>>8)&7)==0);
    for(int i=0;i<nCr;i++){ Cr*c=&cr[i]; if(!c->alive) continue; u16 f=crFl[i];
        if(f==0&&!c->rk&&c->dir<5) continue;                  // (the common case: nothing to look at)
        if(!(f&FL_NOTE)){ int d4=arch[sp[c->sp].a].diet==4; if(c->rk||(d4&&c->dir>=5)) nbPromote(i,(f&FL_KMASK)>>12,(f&FL_BMASK)>>14,1+(c->rk==2)); f=crFl[i]; }
        if(fade&&(f&FL_GRIEF)){ crFl[i]&=(u16)~FL_GRIEF; f=crFl[i]; }
        if(f&FL_FIGHT){ crFl[i]&=(u16)~FL_FIGHT; f=crFl[i];
            if(f&FL_NOTE){ int k=nbFind(crId[i]); if(k>=0){ Nb*n=&nb[k]; if(n->battles<255) n->battles++; if(n->battles>=3){ crFl[i]|=FL_VET; if(!idAxis(crId[i],crFl[i],0)) crFl[i]|=FL_BRAVEN; } } }
            else { int bc=(f&FL_BMASK)>>14; if(bc<3){ bc++; crFl[i]=(u16)((crFl[i]&~FL_BMASK)|(bc<<14)); if(bc>=3) nbPromote(i,(f&FL_KMASK)>>12,3,0); } } } } }

// ---------------------------------------------------------------------------------------------- stars on individuals
static int crStarred(int i){ return i>=0&&i<nCr&&idStarred(crId[i]); }
static int crStarToggle(int i){      // returns 1 when it is now starred. A starred creature gets a record, so it keeps one.
    if(i<0||i>=nCr) return 0; u32 id=crId[i]; for(int k=0;k<CSTAR_N;k++) if(favCrId[k]==id){ favCrId[k]=0; return 0; }
    int put=-1; for(int k=0;k<CSTAR_N;k++) if(!favCrId[k]){ put=k; break; } if(put<0){ memmove(favCrId,favCrId+1,(CSTAR_N-1)*sizeof(u32)); put=CSTAR_N-1; }
    favCrId[put]=id; nbMake(i,60); return 1; }

#endif
