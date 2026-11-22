// ggidsv.h -- the GGN1 save trailer of ggid.h (included once the save layer's SV type and svRead / svWrite exist)
#ifndef GGIDSV_H
#define GGIDSV_H
// ---------------------------------------------------------------------------------------------- the GGN1 save trailer
// magic, id counter, creature count, the ids as gaps (a varint each: almost always one byte), the non-zero flag words as (index gap, u16), the stars, then the notable table.
static void idPut(u8*b,int*p,u32 v){ while(v>=128){ b[(*p)++]=(u8)(v|128); v>>=7; } b[(*p)++]=(u8)v; }
static int idWrite(SV*f){
    u32 mg=0x314E4747u, hd[2]={idNext,(u32)nCr}; int ok=svWrite(&mg,4,1,f)==1&&svWrite(hd,4,2,f)==2; u8 b[512]; int p=0; u32 prev=0;
    for(int i=0;i<nCr&&ok;i++){ idPut(b,&p,crId[i]-prev); prev=crId[i]; if(p>480){ ok&=svWrite(b,1,p,f)==(size_t)p; p=0; } } if(ok&&p){ ok&=svWrite(b,1,p,f)==(size_t)p; } p=0;
    u32 nf=0; for(int i=0;i<nCr;i++) nf+=crFl[i]!=0; if(ok) ok&=svWrite(&nf,4,1,f)==1; int pi=0;
    for(int i=0;i<nCr&&ok;i++) if(crFl[i]){ idPut(b,&p,(u32)(i-pi)); pi=i; b[p++]=(u8)crFl[i]; b[p++]=(u8)(crFl[i]>>8); if(p>480){ ok&=svWrite(b,1,p,f)==(size_t)p; p=0; } } if(ok&&p){ ok&=svWrite(b,1,p,f)==(size_t)p; }
    if(ok) ok&=svWrite(favCrId,4,CSTAR_N,f)==(size_t)CSTAR_N; u32 sz=sizeof(Nb), nn=(u32)nNb; if(ok) ok&=svWrite(&sz,4,1,f)==1&&svWrite(&nn,4,1,f)==1;
    if(ok&&nNb) ok&=svWrite(nb,sizeof(Nb),nNb,f)==(size_t)nNb; return ok; }
static int idGet(SV*f,u32*v){ u32 r=0; int sh=0; for(int k=0;k<5;k++){ u8 c; if(svRead(&c,1,1,f)!=1||(k==4&&(c&0xF0))) return 0; r|=(u32)(c&127)<<sh; if(!(c&128)){ *v=r; return 1; } sh+=7; } return 0; }
// Reads the trailer for the nCr creatures just loaded. 0 = none or bad (the caller then hands out ids in list order).
static int idRead(SV*f){
    u32 mg=0, hd[2]; if(svRead(&mg,4,1,f)!=1||mg!=0x314E4747u||svRead(hd,4,2,f)!=2||hd[1]!=(u32)nCr) return 0;
    u32 prev=0; for(int i=0;i<nCr;i++){ u32 g; if(!idGet(f,&g)||g==0||prev+g<prev){ return 0; } prev+=g; crId[i]=prev; } if(hd[0]<=prev) return 0; idNext=hd[0];
    u32 nf=0; if(svRead(&nf,4,1,f)!=1||nf>(u32)nCr) return 0; for(int i=0;i<nCr;i++) crFl[i]=0; int pi=0; for(u32 k=0;k<nf;k++){ u32 g; u8 w[2]; if(!idGet(f,&g)||svRead(w,1,2,f)!=2) return 0; if(g>=(u32)(nCr-pi)) return 0; pi+=(int)g; crFl[pi]=(u16)(w[0]|w[1]<<8); }
    nNb=0; memset(favCrId,0,sizeof favCrId); u32 fv[CSTAR_N]; if(svRead(fv,4,CSTAR_N,f)!=(size_t)CSTAR_N) return 1; for(int k=0;k<CSTAR_N;k++) favCrId[k]=fv[k];
    u32 sz=0, nn=0; if(svRead(&sz,4,1,f)!=1||sz!=sizeof(Nb)||svRead(&nn,4,1,f)!=1||nn>(u32)NB_N) { for(int i=0;i<nCr;i++) crFl[i]&=(u16)~FL_NOTE; return 1; }
    if(nn&&svRead(nb,sizeof(Nb),nn,f)!=(size_t)nn){ for(int i=0;i<nCr;i++) crFl[i]&=(u16)~FL_NOTE; return 1; } nNb=(int)nn;
    for(int k=1;k<nNb;k++) if(nb[k].id<=nb[k-1].id){ nNb=0; break; }                              // must be sorted and unique, or it is not trusted
    for(int i=0;i<nCr;i++) if((crFl[i]&FL_NOTE)&&nbFind(crId[i])<0) crFl[i]&=(u16)~FL_NOTE;      // a flag with no record
    return 1; }

#endif
