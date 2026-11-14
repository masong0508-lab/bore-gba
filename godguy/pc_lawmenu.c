// The WORLD > LAWS tab on the real program (through compat/ndsshim.h): four pages, every row has a value, a row flips its law, and the page row turns the page.
// gcc -O2 -DNDSSHIM -Icompat -Isource -Wno-all -include unistd.h -o pc_lawmenu pc_lawmenu.c -lm && ./pc_lawmenu
#define UI_NOTRANS 1
#define main ds_main
#include "source/main.c"
#undef main
void shimInput(int frame,int*held,int*tx,int*ty){ *held=0; *tx=*ty=0; }
void shimPresent(void){}
static int fails=0; static void chk(int c,const char*m){ if(!c){ printf("FAIL %s\n",m); fails++; } }
int main(){ gxDefaults();
    int seen[LW_N]={0};
    for(int pg=0;pg<4;pg++){ wTab=4; wLawPg=pg; worldBuild(); chk(wn>=5&&wn<=9,"page row count"); chk(wr[0].act==7,"first row is the page row"); for(int i=0;i<wn;i++){ chk(wVal[i][0]!=0,"row has a value"); for(int k=0;k<LW_N;k++) if(wr[i].p==&lw[k]) seen[k]=1; } }
    for(int k=0;k<LW_N;k++) chk(seen[k],"every law has a row");
    wTab=wTabPrev=4; wLawPg=0; worldBuild(); wSel=2; worldInput(KEY_A); worldBuild(); chk(lw[LW_HUNGER]==1,"A flips HUNGER"); chk(!strcmp(wVal[2],"OFF"),"HUNGER shows OFF");
    wSel=0; worldInput(KEY_RIGHT); chk(wLawPg==1,"RIGHT on the page row turns the page"); worldInput(KEY_LEFT); worldInput(KEY_LEFT); chk(wLawPg==3,"and wraps");
    wLawPg=1; worldBuild(); int gi=-1; for(int i=0;i<wn;i++) if(wr[i].p==&lw[LW_GAIA]) gi=i; chk(gi>=0&&!strcmp(wVal[gi],"OFF"),"GAIA starts OFF"); wSel=gi; worldInput(KEY_A); worldBuild(); chk(lw[LW_GAIA]==1&&!strcmp(wVal[gi],"ON"),"GAIA flips to ON");
    printf("%s\n",fails?"LAW MENU FAIL":"LAW MENU OK"); return fails?1:0; }
