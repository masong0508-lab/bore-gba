// sharecode.h - SHARE CODES: a whole creature (its look, age and persona) as a short code of letters and digits that anyone can type into
// the creator (the full game's or the CREATOR DEMO's: ccdemo.h) to get the very same Bore.
//   IN THE GAME   the creator's DONE tab: SHARE CODE / ENTER A CODE (the code's Bore becomes the one on screen), and HOUSEHOLD > ADD FROM A CODE
//                 (the code's Bore joins your household: hhcreate.h). Like a Spore creature card, but text.
//   The code is one big number in base 32, written with "ABCDEFGHJKLMNPQRSTUVWXYZ23456789" (no I, O, 0 or 1 to mix up), after a first
//   letter that says the code's version ('A'). The number holds, in this order: a check (0..250: a mistyped code is refused, not loaded
//   as some other creature), the age, the aspiration, the lifetime want, the five traits, then every look (each with as many values as the
//   creator offers it). Fields that are 0 add nothing, so a plain Bore has a short code and a wild one a long one (about 80 letters at most).
//   A later version adds fields at the end and gets the next letter; older codes still load (the new fields start at 0).
// Hand-built block bodies (the classic screen) are not in the code: it carries the look the blocks were made from.
// Needs: look, stage, pAsp, pLtw, pTr, lkCnt, BUTT_OFF, fixLook, buildLook, setColors, ageSave, persSave, persValid, the UI kit.
#define SC_ALPH "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"
#define SC_LIMB 72      // bytes of the big number (576 bits; the fields need about 400)
#define SC_MAXC 128     // most letters a code may have
#define SC_NF (1+1+1+1+TR_N+LK_N)   // fields: check, age, aspiration, lifetime want, traits, looks
#define SC_NFA (SC_NF-1)   // the fields of a version 'A' code: everything before VOICE (the last look). A Bore whose voice is BY GENDER still gets an 'A' code, so old and new codes are the same
static int scRad(int f){   // how many values field f has
    if(f==0) return 251;   // (a byte: the fields are kept in u8)
    if(f==1) return AG_N;
    if(f==2) return AS_PICK;
    if(f==3) return LTN;
    if(f<4+TR_N) return 11;
    int id=f-4-TR_N; return id==LK_BUTT?BUTT_OFF+1:lkCnt[id]?lkCnt[id]:9;   // (a look without a count would wipe the number: a slider's 9)
}
static void scVals(u8*v){   // the creature in the creator, as fields (the check left 0)
    v[0]=0; v[1]=stage; v[2]=pAsp<AS_PICK?pAsp:0; v[3]=pLtw<LTN?pLtw:0;
    for(int i=0;i<TR_N;i++) v[4+i]=pTr[i]>10?10:pTr[i];
    for(int i=0;i<LK_N;i++){ int r=scRad(4+TR_N+i); v[4+TR_N+i]=look[i]<r?look[i]:0; }
}
static int scCheckN(const u8*v,int nf){ u32 h=0x2F; for(int f=1;f<nf;f++) h=(h*31u+v[f]+1u)%1000003u; return (int)(h%251u); }
static int scCheck(const u8*v){ return scCheckN(v,SC_NF); }
static void bgMulAdd(u8*b,int m,int a){ u32 c=(u32)a; for(int i=0;i<SC_LIMB;i++){ c+=(u32)b[i]*(u32)m; b[i]=(u8)c; c>>=8; } }
static int bgDiv(u8*b,int d){ u32 r=0; for(int i=SC_LIMB-1;i>=0;i--){ r=(r<<8)|b[i]; b[i]=(u8)(r/(u32)d); r%=(u32)d; } return (int)r; }
static int bgZero(const u8*b){ for(int i=0;i<SC_LIMB;i++) if(b[i]) return 0; return 1; }
static int scMake(char*out){   // the code of the creature in the creator (out: SC_MAXC+1 chars). Returns its length
    u8 v[SC_NF], b[SC_LIMB]; scVals(v); int nf=v[SC_NF-1]?SC_NF:SC_NFA; v[0]=(u8)scCheckN(v,nf);   // ('B': the voice is set)
    for(int i=0;i<SC_LIMB;i++) b[i]=0;
    for(int f=nf-1;f>=0;f--) bgMulAdd(b,scRad(f),v[f]);   // field 0 is the lowest digit: it comes out first when reading
    char d[SC_MAXC]; int n=0; while(!bgZero(b)&&n<SC_MAXC-1) d[n++]=SC_ALPH[bgDiv(b,32)];
    int k=0; out[k++]=nf==SC_NF?'B':'A'; while(n>0) out[k++]=d[--n]; out[k]=0; return k;
}
static int scVal(char c){ if(c>='a'&&c<='z') c=(char)(c-32); for(int i=0;i<32;i++) if(SC_ALPH[i]==c) return i; return -1; }
// Read a code into v (SC_NF fields). Spaces and dashes are skipped. 1 = a sound code
static int scRead(const char*s,u8*v){
    while(*s==' '||*s=='-') s++;
    int nf=(*s=='A'||*s=='a')?SC_NFA:(*s=='B'||*s=='b')?SC_NF:0; if(!nf) return 0;   // 'A': before VOICE; 'B': with it
    s++;
    u8 b[SC_LIMB]; for(int i=0;i<SC_LIMB;i++) b[i]=0;
    for(;*s;s++){ if(*s==' '||*s=='-') continue; int d=scVal(*s); if(d<0||b[SC_LIMB-1]>=8) return 0; bgMulAdd(b,32,d); }
    for(int f=0;f<SC_NF;f++) v[f]=f<nf?(u8)bgDiv(b,scRad(f)):0;
    if(!bgZero(b)||v[0]!=scCheckN(v,nf)) return 0;
    u8 tr[TR_N]; for(int i=0;i<TR_N;i++) tr[i]=v[4+i];
    return persValid(v[2],v[3],tr);
}
static void scLookOf(const u8*v,u8*lk){   // the looks of a code, pulled into what its age offers (fixLook, on a borrowed look[]): for a household member
    u8 keep[LK_N]; int ks=stage; for(int i=0;i<LK_N;i++){ keep[i]=look[i]; look[i]=v[4+TR_N+i]; } stage=v[1];
    fixLook(); for(int i=0;i<LK_N;i++){ lk[i]=look[i]; look[i]=keep[i]; } stage=(u8)ks;
}
static void scApply(const u8*v){   // the creature of a code comes into the creator
    stage=v[1]; pAsp=v[2]; pLtw=v[3]; for(int i=0;i<TR_N;i++) pTr[i]=v[4+i];
    for(int i=0;i<LK_N;i++) look[i]=v[4+TR_N+i];
    fixLook(); custom=0; buildLook(); setColors(); ageSave(); persSave();
}
// ---- the screens ----
static void scDrawCode(int x,int y,const char*c,u16 col){   // in groups of four, six groups to a line
    char ln[32]; int k=0, g=0; const char*p=c;
    for(;*p;p++){ ln[k++]=*p; if(++g%4==0&&p[1]){ if(g%24==0){ ln[k]=0; text(x,y,ln,col,1); y+=9; k=0; } else ln[k++]=' '; } }
    ln[k]=0; if(k) text(x,y,ln,col,1);
}
static void scShow(void){   // SHARE CODE: the code of the creature in the creator, to copy down
    static char c[SC_MAXC+1] EWRAM_BSS; scMake(c);
    u16 prev=keyNow(); int dirty=1;
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k;
        if(pr&(K_A|K_B|K_START)) return;
        if(!dirty){ vsync(); continue; } dirty=0;
        box(3,1,234,157); text(14,10,"SHARE CODE",GOLD,1);
        text(14,24,"TYPE THIS INTO ANY BORE CREATOR",DIMC,1); text(14,32,"(ENTER A CODE) TO GET THIS VERY BORE",DIMC,1);
        rect(10,44,220,84,RGB(5,6,10)); scDrawCode(16,50,c,WHITE);
        text(14,134,"LETTERS AND DIGITS ONLY: NO I O 0 OR 1",DIMC,1);
        text(14,144,"PRESS A TO CLOSE",DIMC,1);
        uiPresent();
    }
}
// ENTER A CODE: a pad of the 32 code letters. 1 = a sound code was entered: its fields are in v (scEnter: the creature is in the creator now)
static int scEnterV(u8*v){
    static char b[SC_MAXC+1] EWRAM_BSS; int n=0; b[0]=0;
    int r=0, c=0, blink=0, bad=0; u16 prev=keyNow(); int hold[10]={0}; u32 drawn=0xFFFFFFFFu;   // (drawn: what the screen shows; it is only drawn again when that changes)
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; blink++; for(int i=0;i<10;i++) hold[i]=(k>>i&1)?hold[i]+1:0;
        #define KREP(m,i) ((pr&(m))||(hold[i]>14&&(hold[i]&3)==0))
        int cols=r<4?8:3;
        if(KREP(K_DOWN,7)) r=(r+1)%5;
        if(KREP(K_UP,6)) r=(r+4)%5;
        cols=r<4?8:3; if(c>=cols) c=cols-1;
        if(KREP(K_RIGHT,4)) c=(c+1)%cols;
        if(KREP(K_LEFT,5)) c=(c+cols-1)%cols;
        #undef KREP
        int key=-1; if(pr&K_A) key=r<4?SC_ALPH[r*8+c]:256+c;
        if(pr&K_B){ if(n>0) b[--n]=0; else return 0; }
        if(pr&K_START) key=256+2;
        if(pr&K_SEL){ r=4; c=2; }
        if(key>=0&&key<256){ if(n<SC_MAXC){ b[n++]=(char)key; b[n]=0; } bad=0; }
        else if(key==256){ if(n>0) b[--n]=0; bad=0; }
        else if(key==257){ n=0; b[0]=0; bad=0; }
        else if(key==258){ if(n&&scRead(b,v)){ sfxPlay(SFX_CASH); return 1; } bad=1; sfxPlay(SFX_POP); }
        { u32 st=(u32)(r|(c<<3)|(n<<6)|(bad<<14)|(((blink>>4)&1)<<15)); if(st==drawn){ vsync(); continue; } drawn=st; }
        box(3,1,234,157); text(14,8,"ENTER A SHARE CODE",GOLD,1);
        { char t[8]; char*e=t; e+=numStr(e,n); *e=0; text(226-tw(t,1),8,t,DIMC,1); }
        rect(10,18,220,46,RGB(5,6,10)); rect(10,63,220,1,GOLD2);
        { const char*sh=b; int lines=(n+23)/24; if(lines>5) sh=b+(lines-5)*24;   // the last five lines show
          static char tmp[SC_MAXC+1] EWRAM_BSS; int q=0; for(;sh[q];q++) tmp[q]=sh[q]; tmp[q]=0; scDrawCode(16,21,tmp,WHITE); }
        if(!n) text(16,21,"START WITH THE LETTER A",DIMC,1);
        if((blink>>4)&1) rect(222,56,4,6,GOLD);
        for(int rr=0;rr<4;rr++) for(int cc=0;cc<8;cc++){
            int kx=22+cc*25, ky=70+rr*15, f=(rr==r&&cc==c); char t[2]={SC_ALPH[rr*8+cc],0};
            rect(kx,ky,21,12,f?RGB(6,16,8):RGB(5,7,14)); if(f) rect(kx,ky+11,21,1,GOLD); text(kx+8,ky+3,t,f?GOLD:WHITE,1); }
        static const char* const sk[3]={"DEL","CLEAR","DONE"};
        for(int i=0;i<3;i++){ int kx=22+i*68, ky=131, f=(r==4&&c==i); rect(kx,ky,62,11,f?RGB(6,16,8):RGB(5,7,14)); if(f) rect(kx,ky+10,62,1,GOLD); text(kx+31-tw(sk[i],1)/2,ky+2,sk[i],f?GOLD:WHITE,1); }
        text(14,147,bad?"THAT CODE DOES NOT WORK  CHECK IT":"A TYPE  B RUB OUT  START DONE",bad?RGB(31,12,10):DIMC,1);
        present();
    }
}
static int scEnter(void){ u8 v[SC_NF]; if(!scEnterV(v)) return 0; scApply(v); return 1; }
