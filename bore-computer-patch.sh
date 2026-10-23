# BORE computer patch: the COMPUTER gets more to do: PLAY A GAME, SURF THE WEB (wiki hole), STUDY ONLINE (logic), MAKE DIGITAL ART (creativity),
# CHAT ONLINE (social) and a FREELANCE GIG (cash, twice a day) next to the job board. Needs bore-jobs-patch.sh applied. Run inside ~/bore-gba.
[ -f source/jobhunt.h ] || { echo "apply bore-jobs-patch.sh first"; exit 1; }
grep -q "jbComp(int k)" source/jobhunt.h && { echo "already applied"; exit 0; }
python3 -I - <<'PYEOF'
import sys
NEW=r'''static void hmNeed(int*v,int d);   // skills.h
static u16 jbCDay; static u8 jbCN[6];   // what you did at the computer today (each has a daily limit, so it is not a free skill farm)
static char jbCM[36] EWRAM_BSS;
static int jbCLimit(int k,int lim){ if(jbCDay!=(u16)(simDay+1)){ jbCDay=(u16)(simDay+1); for(int i=0;i<6;i++) jbCN[i]=0; } if(jbCN[k]>=lim){ lnote="ENOUGH FOR TODAY"; lnoteT=50; return 1; } jbCN[k]++; return 0; }
static void jbComp(int k){   // the computer's things to do
    lsp=0; lgrind=0;
    if(sNrg<10&&k!=0){ lnote="TOO TIRED"; lnoteT=50; return; }
    if(k==0){ if(jbCLimit(0,6)) return; lstun=90; hmNeed(&sCom,10); hmNeed(&sNrg,-5); moodEvent(M_SOFA); simEvent(SE_LAUGH); sfxPlay(SFX_TV); lnote="PLAYED A GAME  FUN"; lnoteT=60; if(simMin>=SIM_NIGHT_FROM||simMin<SIM_NIGHT_TO) hmNeed(&sNrg,-6); }
    else if(k==1){ if(jbCLimit(1,3)) return; lstun=100; hmNeed(&sCom,5); skGain(SK_LOGIC,1);
        if((rnd8()&7)==0){ lstun=260; skGain(SK_LOGIC,2); hmNeed(&sNrg,-6); lnote="YOU FELL DOWN A WIKI HOLE"; } else lnote="SURFED THE WEB"; lnoteT=70; sfxPlay(SFX_PAGE); }
    else if(k==2){ if(jbCLimit(2,3)) return; lstun=140; hmNeed(&sNrg,-4); skGain(SK_LOGIC,2); simEvent(SE_READ); sfxPlay(SFX_PAGE); lnote="STUDIED ONLINE  LOGIC UP"; lnoteT=60; }
    else if(k==3){ if(jbCLimit(3,3)) return; lstun=140; hmNeed(&sNrg,-4); hmNeed(&sCom,4); skGain(SK_CREAT,2); moodEvent(M_SOFA); sfxPlay(SFX_POP); lnote="MADE DIGITAL ART  CREATIVITY UP"; lnoteT=70; }
    else if(k==4){ if(jbCLimit(4,3)) return; lstun=100; hmNeed(&sSoc,12); skGain(SK_CHARM,1); hmNeed(&sCom,4); moodEvent(M_SOFA); sfxPlay(SFX_POP); lnote="CHATTED ONLINE  SOCIAL UP"; lnoteT=60; }
    else if(k==5){ if(sNrg<25){ lnote="TOO TIRED FOR A GIG"; lnoteT=50; return; } if(jbCLimit(5,2)) return;
        int pay=35+(skLvl(SK_LOGIC)+skLvl(SK_CREAT))*6; lstun=160; hmNeed(&sNrg,-15); simMoneyAdd(pay); moodEvent(M_PAY); sfxPlay(SFX_POP);
        char*e=simCat(jbCM,"FREELANCE GIG  +\xC2\xA7"); simCatN(e,pay); lnote=jbCM; lnoteT=70; }
}
static int jbUse(void){   // R next to a COMPUTER. 1 = handled
    if(!ojobOn()&&stage<AG_CHILD) return 0;
    if(!jbCompNear()) return 0;
    static char tt[32] EWRAM_BSS; char*e=simCat(tt,"COMPUTER  \xC2\xA7"); simCatMoney(e,simMoney,1);
    const char* it[8]; int id[8], n=0;
    if(ojobOn()){ it[n]="JOB BOARD  ALL JOBS"; id[n++]=10; }
    it[n]="PLAY A GAME"; id[n++]=0; it[n]="SURF THE WEB"; id[n++]=1; it[n]="STUDY ONLINE"; id[n++]=2; it[n]="MAKE DIGITAL ART"; id[n++]=3; it[n]="CHAT ONLINE"; id[n++]=4;
    if(stage>=AG_TEEN){ it[n]="FREELANCE GIG  PAYS"; id[n++]=5; }
    if(ojobOn()){ it[n]="VIEW MY CAREER"; id[n++]=11; }
    int c=menu(tt,it,n);
    liveInvalidate(); camSnap=1; while((~REG_KEYINPUT)&0x3FF) vsync();
    if(c<0) return 1;
    if(id[c]==10){ int al[JT_N], na=jbAllowedList(al); if(na>0) jbBoard("JOB BOARD",al,na); liveInvalidate(); camSnap=1; while((~REG_KEYINPUT)&0x3FF) vsync(); }
    else if(id[c]==11){ careerScreen(); liveInvalidate(); camSnap=1; while((~REG_KEYINPUT)&0x3FF) vsync(); }
    else jbComp(id[c]);
    return 1;
}
'''
s=open("source/jobhunt.h").read()
a=s.find("static int jbUse(void){"); b=s.find("static const char* jbHint")
if a<0 or b<a: print("jbUse block not found"); sys.exit(1)
open("source/jobhunt.h","w").write(s[:a]+NEW+s[b:])
PYEOF
[ $? -eq 0 ] || exit 1
echo "computer patch applied"
