// kingslot.h - EP6 THE LAND BOREFORE TIME: a REALM in the save slots (kind SLK_KING). Roadmap Phase 2, step b.
// kingdom.h turns a realm into bytes; this puts those bytes in slots and gets them back, the way savegame.h does for a player.
// The KINGDOM screen (kingscreen.h) calls it; every function is static, so an unused one costs no ROM.
//
// A realm's slot run:  header (32 B, slots.h)  then the payload:  'K', length (u16), the kgEncode bytes, 0 (the end of the chunks).
//   header byte 28 = the REALM'S NUMBER (1..250), like a player's number: it is what ties a realm's copies together.
//   the slot name is the realm's name (10 letters).  has = 0.
//
// SAVING IS A FRESH COPY (roadmap 5.4, the biggest trust liability): the new copy is written into free slots, read back and checked, and only then is
// the old copy dropped. A power cut at ANY point leaves the old copy whole (the new one is invisible until its header goes in, last) or, in the one
// moment when both are whole, two copies of the same realm: the newer (wrap-safe seq) wins and kgsHeal drops the older. There is NO save over itself:
// if no free run is big enough the save says SLE_NOROOM and touches nothing (step e reserves room so that a realm can always be saved).
//
// Needs before it: slots.h (slInfo, slScan, slOwner, slI, slGood, slOpen, slHeader, slVerify, slFits, slSumOf, slHPid, SLE_*), kingdom.h.

#define KGS_MAXSPAN 6          // slots a realm may span (12 KB: the measured worst case was 7.7 KB with 40 buildings; with 64 it is about 10 KB; roadmap 5.4)
#define KGS_LISTMAX 16         // realms kgsList can return
#define KGS_PRE     3          // payload bytes before the realm's bytes: 'K' and a u16 length

static int kgsErr(int e){      // a kingdom.h error as a slot error
    return e==KGE_BIG?SLE_BIG:e==KGE_SUM?SLE_BAD:e==KGE_ARG?SLE_BAD:SLE_FMT;
}
static int kgsNewer(const SlInfo*a,const SlInfo*b){ return (short)(a->seq-b->seq)>0; }   // a was saved after b (seq wraps)

// the slot that holds the newest good copy of realm id, or -1. Needs slScan to be fresh.
static int kgsFind(int id){
    int best=-1;
    for(int i=0;i<SLOT_N;i++) if(slOwner[i]==i&&slI[i].kind==SLK_KING&&slGood[i]&&slI[i].pid==id&&(best<0||kgsNewer(&slI[i],&slI[best]))) best=i;
    return best;
}
static int kgsNewId(void){     // the lowest realm number no slot uses (damaged copies count: their number is not free to take)
    for(int id=1;id<250;id++){ int used=0; for(int i=0;i<SLOT_N;i++) if(slOwner[i]==i&&slI[i].kind==SLK_KING&&slI[i].pid==id) used=1; if(!used) return id; }
    return 0;
}
// the realms there are: the slot of the newest good copy of each, in slot order. Returns how many (at most max). Needs slScan to be fresh.
static int kgsList(int*l,int max){
    int n=0;
    for(int i=0;i<SLOT_N&&n<max;i++){
        if(slOwner[i]!=i||slI[i].kind!=SLK_KING||!slGood[i]) continue;
        if(kgsFind(slI[i].pid)==i) l[n++]=i;
    }
    return n;
}
// drop every older copy of realm id that is left over (a power cut between "the new copy is whole" and "the old one goes"). Returns how many it dropped.
static int kgsHeal(int id){
    int keep=kgsFind(id), n=0; if(keep<0) return 0;
    for(int i=0;i<SLOT_N;i++) if(i!=keep&&slOwner[i]==i&&slI[i].kind==SLK_KING&&slI[i].pid==id&&slGood[i]){ svWr(SLO(i),0); svWr(SLO(i)+1,0); n++; }
    if(n) svCommit();
    return n;
}
// what a realm is, without loading it: reads the header bytes of its payload. 1 = ok. (the KINGDOM screen's list rows)
static int kgsPeek(int slot,int*tier,int*giant,int*castle,u32*day){
    SlInfo I; if(!slInfo(slot,&I)||I.kind!=SLK_KING||I.len<KGS_PRE+KG_HDR+2) return 0;
    volatile u8*p=SLB(slot)+SLOT_HDR+KGS_PRE; if(p[0]!=KG_FMT&&p[0]!=KG_FMT1) return 0;
    if(tier){ *tier=p[2]; }
    if(giant){ *giant=p[3]; }
    if(castle){ *castle=p[4]; }
    if(day){ *day=(u32)p[10]|((u32)p[11]<<8)|((u32)p[12]<<16)|((u32)p[13]<<24); }
    return 1;
}

// SAVE: realm id (0 = a new realm) with its map, buildings and numbers. buf (cap bytes) is a scratch for the encoded realm: the caller owns it
// (the realm's map lives in a shared scratch, roadmap 5.1, so no new static is made here). name: 10 letters, or 0 to keep the realm's own.
// Returns the slot it now lives in (>= 0), or a negative SLE_ error with every earlier copy untouched. *idOut gets the realm's number.
static int kgsSaveI(int id,const char*name,const KRealm*R,const KBld*B,const u8*cells,u8*buf,int cap,int*idOut){
    slScan();
    int old=id?kgsFind(id):-1;
    if(id&&old<0) return SLE_EMPTY;                                   // a realm that is not there cannot be saved over
    int cmax=KGS_MAXSPAN*SLOT_SZ-SLOT_HDR-KGS_PRE-1; if(cap>cmax) cap=cmax;
    int n=kgEncode(buf,cap,R,B,cells); if(n<0) return kgsErr(n);
    int len=KGS_PRE+n+1, span=(SLOT_HDR+len+SLOT_SZ-1)/SLOT_SZ;
    if(span>KGS_MAXSPAN) return SLE_BIG;
    int dst=-1;
    for(int s=0;s+span<=SLOT_N&&dst<0;s++){ if(!slFits(s,span)) continue; int f=1; for(int k=0;k<span;k++) if(slOwner[s+k]>=0) f=0; if(f) dst=s; }   // free slots only: never over the old copy
    if(dst<0) return SLE_NOROOM;
    if(!id){ id=kgsNewId(); if(!id) return SLE_NOROOM; }
    char nm[SLOT_NAME+1]; int i=0;
    if(name){ for(;name[i]&&i<SLOT_NAME;i++) nm[i]=name[i]; }
    else if(old>=0){ for(;i<SLOT_NAME&&slI[old].name[i];i++) nm[i]=slI[old].name[i]; }
    if(!i){ nm[0]='R'; nm[1]='E'; nm[2]='A'; nm[3]='L'; nm[4]='M'; i=5; }
    nm[i]=0;
    int seq=old>=0?slI[old].seq+1:1;
    slOpen(dst,span);                                                 // empty while we write
    u32 o=SLO(dst)+SLOT_HDR; u32 s1=0,s2=0; int pos=0;
    #define KGS_PUT(b) do{ int _b=(b)&255; svWr(o+(u32)pos,_b); pos++; s1+=(u32)_b; if(s1>=255) s1-=255; s2+=s1; if(s2>=255) s2-=255; }while(0)
    KGS_PUT('K'); KGS_PUT(n&255); KGS_PUT(n>>8);
    for(int k=0;k<n;k++) KGS_PUT(buf[k]);
    KGS_PUT(0);
    #undef KGS_PUT
    slHPid=(u8)id; slHeader(dst,SLK_KING,span,0,len,(int)(s1|(s2<<8)),seq,nm);   // (the header goes in last: until then the new copy is an empty slot)
    int e=slVerify(dst);
    if(e){ svWr(SLO(dst),0); svWr(SLO(dst)+1,0); svCommit(); return e; }       // it did not read back: forget it, the old copy stays
    if(old>=0){ svWr(SLO(old),0); svWr(SLO(old)+1,0); }                // the old copy goes only now
    svCommit();
    if(idOut) *idOut=id;
    return dst;
}
static int kgsSave(int id,const char*name,const KRealm*R,const KBld*B,const u8*cells,u8*buf,int cap,int*idOut){   // flash writes are slow: a loading screen covers them
    ldShow("SAVING THE KINGDOM",0,2); int e=kgsSaveI(id,name,R,B,cells,buf,cap,idOut); ldShow("SAVING THE KINGDOM",2,2); ldEnd(); return e;
}

// LOAD: checks the stored realm first (apply 0) and only then fills R, B (room for KG_MAXB) and cells, so a bad save never half-replaces a realm in play.
// Reads straight from the save memory: tmp (cap bytes, at least as big as the realm's bytes) holds the copy kgDecode reads.
static int kgsLoad(int slot,KRealm*R,KBld*B,u8*cells,u8*tmp,int cap){
    SlInfo I; if(!slInfo(slot,&I)||I.kind!=SLK_KING) return SLE_EMPTY;
    volatile u8*s=SLB(slot)+SLOT_HDR;
    if(slSumOf(s,I.len)!=I.sum) return SLE_BAD;
    if(I.len<KGS_PRE+KG_HDR+2+1||s[0]!='K') return SLE_FMT;
    int n=s[1]|(s[2]<<8);
    if(KGS_PRE+n+1!=I.len||s[KGS_PRE+n]!=0) return SLE_FMT;
    if(n>cap) return SLE_BIG;
    for(int i=0;i<n;i++) tmp[i]=s[KGS_PRE+i];
    int e=kgDecode(tmp,n,R,B,cells,0); if(e) return kgsErr(e);       // pass 1: check only
    e=kgDecode(tmp,n,R,B,cells,1); if(e) return kgsErr(e);           // pass 2: load
    return SLE_OK;
}

// RENAME a realm: the same fresh-copy way as a save, but the payload bytes are copied as they stand (no map in RAM, nothing decoded), so only the header's
// name and seq change. The copy is written into free slots, read back, and only then is the old one dropped: a cut leaves the old name or the new, whole.
// Returns the slot it now lives in (>= 0), or a negative SLE_ error with the realm untouched (SLE_NOROOM when no free run is big enough: the reserve of step e keeps one).
static int kgsRenameI(int id,const char*name){
    slScan();
    int old=kgsFind(id); if(old<0) return SLE_EMPTY;
    SlInfo I=slI[old]; int len=I.len, span=(SLOT_HDR+len+SLOT_SZ-1)/SLOT_SZ;
    if(span>KGS_MAXSPAN) return SLE_BIG;
    volatile u8*src=SLB(old)+SLOT_HDR;
    if(slSumOf(src,len)!=I.sum) return SLE_BAD;                        // never copy a damaged realm
    int dst=-1;
    for(int s=0;s+span<=SLOT_N&&dst<0;s++){ if(!slFits(s,span)) continue; int f=1; for(int k=0;k<span;k++) if(slOwner[s+k]>=0) f=0; if(f) dst=s; }
    if(dst<0) return SLE_NOROOM;
    char nm[SLOT_NAME+1]; int i=0;
    if(name){ for(;name[i]&&i<SLOT_NAME;i++) nm[i]=name[i]; }
    if(!i){ for(;i<SLOT_NAME&&I.name[i];i++) nm[i]=I.name[i]; }       // (an empty name keeps the old one)
    nm[i]=0;
    slOpen(dst,span);                                                  // empty while we write
    u32 o=SLO(dst)+SLOT_HDR; for(int k=0;k<len;k++) svWr(o+(u32)k,src[k]);
    slHPid=(u8)id; slHeader(dst,SLK_KING,span,0,len,I.sum,I.seq+1,nm);   // (the header goes in last)
    int e=slVerify(dst);
    if(e){ svWr(SLO(dst),0); svWr(SLO(dst)+1,0); svCommit(); return e; }
    svWr(SLO(old),0); svWr(SLO(old)+1,0); svCommit();                  // the old copy goes only now
    return dst;
}
static int kgsRename(int id,const char*name){ ldShow("RENAMING THE KINGDOM",0,2); int e=kgsRenameI(id,name); ldShow("RENAMING THE KINGDOM",2,2); ldEnd(); return e; }

// DELETE a realm: every copy of it. Returns how many slots were freed.
static int kgsDelete(int id){
    slScan(); int n=0;
    for(int i=0;i<SLOT_N;i++) if(slOwner[i]==i&&slI[i].kind==SLK_KING&&slI[i].pid==id){ svWr(SLO(i),0); svWr(SLO(i)+1,0); n++; }
    if(n) svCommit();
    return n;
}
