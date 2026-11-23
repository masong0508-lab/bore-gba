// Exercise the shared sandbox actions and DS-style controls on the host shim.
#define main ds_main
#include "source/main.c"
#undef main

void shimInput(int frame,int*held,int*x,int*y){ *held=0; *x=*y=0; }
void shimPresent(void){}

static void check(int ok,const char *message){
    if(ok) return;
    fprintf(stderr,"FAIL: %s\n",message);
    exit(1);
}

int main(void){
    mapPtr=(u16*)bgGetMapPtr(0);
    mapPtr1=(u16*)bgGetMapPtr(1);
    makeSpecies();
    makeItems();
    cLife=0;
    newWorld();
    memset(world,T_GRASS,sizeof world);
    bioClear();

    check(!LAWON(LW_RELIEF)&&!LAWON(LW_GREENBELT),"new policies default to off");
    int field=-1,construction=-1,special=-1;
    for(int i=0;i<PW_N;i++){
        if(pw[i].kind==PK_FIELD)field=i;
        if(pw[i].kind==PK_BUILD)construction=i;
        if(pw[i].kind==PK_SPECIAL)special=i;
    }
    check(field>=0&&construction>=0&&special>=0,"find Field Tools, Build and Special Event");
    power=field;
    tbBuild();
    check(tbN<12&&tbPos(power)>=0,"field tools are in the starter kit");
    check(tbPos(construction)<0&&tbHome(construction)==4,"construction starts in Folk");
    tbStep(1);
    check(pw[power].kind==PK_INSPECT,"R wraps inside the Kit");
    tbStep(-1);
    check(pw[power].kind==PK_FIELD,"L wraps inside the Kit");
    for(int i=0;i<PW_N;i++){
        check((unsigned)uiIconForKind(pw[i].kind)<sizeof icon10/sizeof *icon10,"every power has a valid icon");
        tbCat=tbHome(i);
        tbBuild();
        check(tbPos(i)>=0,"every power is reachable from its tab");
    }
    tbCat=0;
    favPow[special]=1;
    tbBuild();
    check(tbPos(special)>=0,"starred powers appear in Kit");
    tbCat=0;
    tbBuild();

    int city=vilFound(1,47,50), neighbor=vilFound(1,65,50);
    check(city>=0&&neighbor>=0,"set up nearby cities");
    putBld(city,40,51,B_HOUSE);
    titleOn=paused=0;
    fieldMode=FIELD_SURVEY;
    usePower(40,51,1);
    check(fieldLedgerOpen&&extrasView&&paused&&wTab==2&&wCity==city,"survey opens the building owner's ledger");
    check(closeFieldLedger(KEY_B)&&!paused&&!extrasView,"survey returns to the running map");
    paused=1;
    usePower(47,50,1);
    check(closeFieldLedger(KEY_START)&&paused,"survey preserves an already paused map");

    int person=addCr(1,47,51), visitor=addCr(2,48,51);
    check(person>=0&&visitor>=0,"stage two creatures");
    cr[person].food=cr[visitor].food=50;
    vil[city].stock=5;
    fieldMode=FIELD_RATIONS;
    usePower(47,50,0);
    check(vil[city].stock==5,"holding the stylus doesn't spend rations");
    usePower(47,50,1);
    check(vil[city].stock==4&&cr[person].food==85&&cr[visitor].food==50,"rations spend city stock on eligible folk");

    cr[person].sick=1|ST_ASH_FEVER|ST_COUGH|ST_INVINCIBLE;
    fieldMode=FIELD_CLINIC;
    usePower(47,50,1);
    check(cr[person].sick==(2|ST_INVINCIBLE),"clinic cures disease and preserves unrelated status");

    setCell(47,52,T_DIRT);
    bioSet(47,52,1);
    bld[52][47]=B_RUIN;
    bioSet(47,50,2);
    fieldMode=FIELD_RESTORE;
    usePower(47,52,1);
    check(world[52][47]==T_GRASS&&bioAt(47,52)==0&&bld[52][47]==B_NONE,"restore clears ruined soil");
    check(bld[50][47]==B_HALL&&bioAt(47,50)==2,"restore preserves occupied infrastructure");

    vil[city].stock=12; vil[neighbor].stock=250;
    int k=kdFound(city), other=kdFound(neighbor);
    check(k>=0&&other>=0&&other!=k,"stage two kingdoms");
    kd[k].allies|=(u16)(1u<<other);
    check(!villageRelief(city),"one-sided alliances cannot send supplies");
    kd[other].allies|=(u16)(1u<<k);
    check(villageRelief(city)==32&&vil[city].stock==44&&vil[neighbor].stock==218,"allied relief conserves stock and caps a shipment");
    kd[other].war|=(u16)(1u<<k);
    check(!villageRelief(city)&&vil[city].stock==44,"wartime rivals withhold relief");

    for(int y=44;y<=56;y++) for(int x=41;x<=53;x++) setCell(x,y,T_DIRT);
    vil[city].farms=1;
    u32 rng=rs;
    check(villageGreenbelt(city)==1&&rs==rng,"greenbelt restores land without drawing RNG");
    int restored=0;
    for(int y=44;y<=56;y++) for(int x=41;x<=53;x++) restored+=world[y][x]==T_GRASS;
    check(restored==1,"one greenbelt site on each city turn");

    wTab=wTabPrev=5;
    camX=90; camY=65;
    wSel=1; worldInput(KEY_A);
    check(mapPinMask==1,"pin is stored through world menu");
    camX=131; camY=153;
    wSel=2; worldInput(KEY_A);
    check(camX==90&&camY==65&&!extrasView,"pin recall returns the camera to the saved view");
    wSel=3; worldInput(KEY_A);
    check(!mapPinMask,"clear removes the pin");

    // Run from a temporary directory like the other shim benchmarks.
    const char *save="field-test.sav";
    lw[LW_RELIEF]=lw[LW_GREENBELT]=favPow[field]=favPow[construction]=1;
    check(saveGameRaw(save),"field save is writable");
    gxDefaults();
    favPow[field]=favPow[construction]=0;
    mapPinMask=1;
    check(loadGameRaw(save),"field save is readable");
    remove(save);
    check(LAWON(LW_RELIEF)&&LAWON(LW_GREENBELT)&&favPow[field]&&favPow[construction],
          "policy and power favorites round-trip");
    check(!mapPinMask,"session pins reset when another world is loaded");

    stUse=0;
    startWorld(0);
    check(tbCat==0&&pw[power].kind==PK_PAINT&&pw[power].arg==T_GRASS,"new worlds start at the Grass tool in Kit");
    check(starterHint()!=NULL&&spSel==0,"opening hint explains the next action and SPAWN defaults to folk");
    check((unsigned)camX<=WW*8-256&&(unsigned)camY<=WH*8-192,"starter camera stays inside the world");
    check(tbStamp==tbDrawKey(),"toolbar draw key is current");
    puts("FIELD TOOLS, CARE POLICIES AND MAP PINS OK");
    return 0;
}
