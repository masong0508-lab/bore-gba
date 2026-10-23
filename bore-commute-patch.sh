# BORE commute patch: every job has a workplace lot (cinema, park, gym, bank, casino, arms shop, library, TACO BORE, skate park); the shift only counts there.
# Get there from the phone (COMMUTE) or YOUR ACTIONS > MIND: ON MY OWN BORE (free, costs energy) or the CARPOOL (5, a big cheap van that drives up). Needs the jobs patch. Run inside ~/bore-gba.
[ -f source/jobhunt.h ] || { echo "apply bore-jobs-patch.sh first"; exit 1; }
grep -q "jbCommute" source/jobhunt.h && { echo "already applied"; exit 0; }
python3 -I - <<'PYEOF'
import sys
NEW=r'''// ---- THE COMMUTE: every job has a workplace lot and the shift only counts while you are there ----
static const u8 jbWk[JT_N]={CT_CINEMA,CT_PARK,CT_GYM,CT_BANK,CT_CASINO,CT_ARMS,CT_LIBRARY,CT_TACO,CT_SKATE};   // the workplace of each track (enum order: ENT SLACK ATH BUS CRIM MIL EDU FOOD SKATE)
static int jbWorkOk(void){ return !nbOk||prIn()||trvHere(jbWk[jobTrack]); }   // no town: work anywhere, as before
static void jbVan(void){   // the CARPOOL: a very cheap van, and very large for what it is: seven passengers and you
    static const u16 hc[7]={RGB(26,14,10),RGB(10,22,12),RGB(10,14,28),RGB(28,24,8),RGB(24,10,24),RGB(8,24,24),RGB(28,16,8)};
    u16 pk=keyNow(); sfxPlay(SFX_CASH);
    for(int f=0;f<150;f++){
        u16 k=keyNow(); if(k&~pk&(K_A|K_B|K_START)) break; pk=k;
        int x; if(f<50){ int r=50-f; x=40-r*r*210/2500; } else if(f<90) x=40; else { int r=f-90; x=40+r*r*220/3600; }
        rect(0,52,SW,56,RGB(13,22,30)); rect(0,108,SW,34,RGB(5,5,7)); rect(0,108,SW,2,RGB(14,14,16));
        for(int d=-24;d<SW;d+=24) rect(d+24-((f*4)%24),124,12,2,RGB(24,24,10));
        if(f>=45&&f<80){ int px=14+(f-45>25?25:f-45)*26/25; rect(px,94,8,12,RGB(8,26,14)); rect(px+1,96,2,2,RGB(2,2,2)); rect(px+5,96,2,2,RGB(2,2,2)); }   // you walk up to the door
        s2rr(x,78,160,28,RGB(28,24,4)); s2rr(x+6,64,118,18,RGB(28,24,4)); rect(x,92,160,3,RGB(26,6,6));
        for(int i=0;i<7;i++){ rect(x+12+i*16,68,14,11,RGB(8,14,22)); rect(x+15+i*16,71,8,8,hc[i]); rect(x+16+i*16,73,1,2,RGB(2,2,2)); rect(x+21+i*16,73,1,2,RGB(2,2,2)); }
        rect(x+126,70,30,12,RGB(8,14,22)); if(f>=80){ rect(x+132,72,8,8,RGB(8,26,14)); rect(x+133,74,1,2,RGB(2,2,2)); rect(x+138,74,1,2,RGB(2,2,2)); }
        rect(x+156,92,4,5,RGB(31,31,18)); rect(x+3,103,154,4,RGB(14,14,16));
        rect(x+20,100,18,14,RGB(3,3,4)); rect(x+25,105,8,5,RGB(14,14,16)); rect(x+112,100,18,14,RGB(3,3,4)); rect(x+117,105,8,5,RGB(14,14,16));
        if(x>=2&&x+110<=SW) text(x+28,83,"CARPOOL",RGB(3,3,3),1);
        if(f<50||f>=90) rect(x-6-(f&7),100-(f&3),5,4,RGB(20,20,22));
        present(); }
    sfxPlay(SFX_POP);
}
static void jbGoTrip(int lot,int mode){   // mode 0: on your own bore (free, tires you), 1: the carpool (5, social)
    if(mode==1&&simMoney<5){ toast("NOT ENOUGH FOR THE FARE"); return; }
    if(mode==0&&sNrg<15){ toast("TOO TIRED TO GET THERE"); return; }
    if(!trvGo(lot)) return;
    if(mode==1){ simMoneyAdd(-5); jbVan(); hmNeed(&sSoc,8); moodEvent(M_SOFA); toast("CARPOOL  PACKED BUT CHEAP"); }
    else { hmNeed(&sNrg,lhave?-6:-12); toast(lhave?"RODE YOUR BOARD THERE":"WALKED THERE"); }
}
static void jbCommute(void){   // the phone and YOUR ACTIONS: to work, or home
    if(!ojob()||!nbOk||prIn()){ toast("NO COMMUTE RIGHT NOW"); return; }
    int wct=jbWk[jobTrack], at=jbWorkOk(), n=0, id[2]; const char* it[2]; static char a[32] EWRAM_BSS;
    if(!at){ char*e=simCat(a,"GO TO WORK  "); simCat(e,ctNm[wct]); it[n]=a; id[n++]=0; }
    if(nbT.home<NB_LOTS&&nbT.home!=nbT.cur){ it[n]="GO HOME"; id[n++]=1; }
    if(!n){ toast("YOU ARE AT WORK  KEEP AT IT"); return; }
    int c=menu("COMMUTE",it,n); if(c<0) return;
    int lot=id[c]==0?trvPlace(wct):nbT.home; if(lot<0){ toast("NO WORKPLACE IN TOWN"); return; }
    static const char* const md[2]={"ON MY OWN BORE  FREE","CARPOOL  \xC2\xA7" "5  BIG VAN"};
    int m=menu("HOW TO GET THERE",md,2); if(m<0) return;
    jbGoTrip(lot,m);
}
'''
F={n:open("source/"+n).read() for n in ["jobhunt.h","sims.h","households.h","self.h"]}
def req(f,old,new,label):
    if F[f].count(old)!=1: print("anchor not found once:",f,label); sys.exit(1)
    F[f]=F[f].replace(old,new)
a=F["jobhunt.h"].find("static void jbDeliver(void){")
if a<0: print("jobhunt.h: jbDeliver not found"); sys.exit(1)
F["jobhunt.h"]=F["jobhunt.h"][:a]+NEW+F["jobhunt.h"][a:]
req("sims.h","static void jbDeliver(void);   // jobhunt.h","static void jbDeliver(void); static int jbWorkOk(void);   // jobhunt.h","fwd")
req("sims.h","simQueue((jobTrack!=JT_SKATE&&simAtWork()==1)?","simQueue(!jbWorkOk()?\"SHIFT TIME  COMMUTE FROM THE PHONE\":(jobTrack!=JT_SKATE&&simAtWork()==1)?","msg")
req("sims.h","if(jobTrack!=JT_SKATE&&simInShift()&&!simAct&&!ldead&&simAtWork()!=1) shiftPts+=","if(jobTrack!=JT_SKATE&&simInShift()&&!simAct&&!ldead&&simAtWork()!=1&&jbWorkOk()) shiftPts+=","minutes")
req("sims.h","if(lscore>simLastScore&&simInShift()&&jobTrack==JT_SKATE) shiftPts+=","if(lscore>simLastScore&&simInShift()&&jobTrack==JT_SKATE&&jbWorkOk()) shiftPts+=","tricks")
req("households.h","static void careerScreen(void);   // career.h","static void careerScreen(void);   // career.h\nstatic void jbCommute(void);   // jobhunt.h","hh fwd")
req("households.h",r'    it[n]="ORDER PIZZA  \xC2\xA7" "80"; id[n++]=1;','    if(ojob()&&nbOk&&!prIn()){ it[n]="COMMUTE  WORK OR HOME"; id[n++]=9; }   // jobhunt.h\n    it[n]="ORDER PIZZA  \\xC2\\xA7" "80"; id[n++]=1;',"phone item")
req("households.h","        case 4: careerScreen(); break;","        case 4: careerScreen(); break;\n        case 9: jbCommute(); break;","phone case")
req("self.h",'        if(c==1&&jbPaperOk()&&q<8){','        if(c==1&&ojob()&&nbOk&&!prIn()&&q<8){ l[q]="COMMUTE  WORK OR HOME"; col[q]=slfCatCol[c]; id[q++]=-15; }   // jobhunt.h\n        if(c==1&&jbPaperOk()&&q<8){',"self item")
req("self.h","else if(pick==-14) jbPaper();","else if(pick==-14) jbPaper(); else if(pick==-15) jbCommute();","self do")
for n in F: open("source/"+n,"w").write(F[n])
PYEOF
[ $? -eq 0 ] || exit 1
echo "commute patch applied"
