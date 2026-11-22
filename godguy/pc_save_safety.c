// Player-data safety through the real menu and save loader.
// gcc -O2 -DNDSSHIM -Icompat -Isource -include unistd.h pc_save_safety.c -lm -o /tmp/gg_save_safety
#define _POSIX_C_SOURCE 200809L
#define UI_NOTRANS 1
#define main ds_main
#include "source/main.c"
#undef main
#include <sys/stat.h>
#include <stddef.h>
#include <unistd.h>
static int fails;
#define CHECK(c,m) do{if(!(c)){fprintf(stderr,"FAIL: %s\n",m);fails++;}}while(0)
void shimInput(int frame,int*held,int*tx,int*ty){(void)frame;*held=0;*tx=*ty=0;}
void shimPresent(void){}
static u32 stateHash(void){
    u32 hash=2166136261u;
#define HASH(p,n) do{const u8*bytes=(const u8*)(p);for(size_t z=0;z<(size_t)(n);z++)hash=(hash^bytes[z])*16777619u;}while(0)
    HASH(&gW,sizeof gW);HASH(&gH,sizeof gH);HASH(&nCr,sizeof nCr);
    for(int y=0;y<gH;y++){HASH(world[y],gW);HASH(bld[y],gW);HASH(bldv[y],gW);}
    HASH(cr,nCr*sizeof *cr);HASH(crId,nCr*sizeof *crId);HASH(crFl,nCr*sizeof *crFl);
    HASH(vil,sizeof vil);HASH(kd,sizeof kd);HASH(kx,sizeof kx);HASH(&gx,sizeof gx);
    HASH(&tick,sizeof tick);HASH(&year,sizeof year);HASH(&chapter,sizeof chapter);
    HASH(&worldSeed,sizeof worldSeed);HASH(&rs,sizeof rs);HASH(favPow,sizeof favPow);
    HASH(&trSeenHi,sizeof trSeenHi);HASH(rz,sizeof rz);HASH(&rzN,sizeof rzN);
#undef HASH
    return hash;
}
static u8*readFile(const char*path,size_t*n){
    FILE*f=fopen(path,"rb");if(!f)return NULL;
    if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}
    long len=ftell(f);if(len<0){fclose(f);return NULL;}
    rewind(f);u8*b=malloc((size_t)len);
    if(!b||fread(b,1,(size_t)len,f)!=(size_t)len){free(b);fclose(f);return NULL;}
    fclose(f);*n=(size_t)len;return b;
}
static int writeFile(const char*path,const void*data,size_t n){
    FILE*f=fopen(path,"wb");if(!f)return 0;
    int ok=fwrite(data,1,n,f)==n;return fclose(f)==0&&ok;
}
static void rejectRecordChange(u8*data,size_t size,size_t at,const void*value,
                               size_t bytes,u32 before,const char*message){
    u8 original[sizeof(Kd)];
    CHECK(bytes<=sizeof original&&at+bytes<=size,"record mutation is inside fixture");
    if(bytes>sizeof original||at+bytes>size)return;
    memcpy(original,data+at,bytes);memcpy(data+at,value,bytes);
    CHECK(writeFile("broken.sav",data,size)&&!loadGame("broken.sav"),message);
    CHECK(stateHash()==before,"invalid record leaves the current world intact");
    memcpy(data+at,original,bytes);
}
static void confirmTitle(void){
    titleOn=0;menuOn=1;menuAsk=8;dlgPaused=0;paused=1;menuInput(KEY_A);
}
int main(void){
    char temp[]="/tmp/gg-save-safety-XXXXXX";
    if(!mkdtemp(temp)||chdir(temp))return 1;
    mapPtr=(u16*)bgGetMapPtr(0);planetSize=128;rs=0x517EAu;
    makeSpecies();makeItems();newWorld();compact();wgBuild();
    year=21;tick=4321;chapter=4;trSeenHi=3;favPow[7]=1;favPow[65]=1;
    CHECK(saveGameRaw("fixture.sav"),"create complete save fixture");
    u32 before=stateHash();
    sdOk=0;savedYear=-1;hasSave=0;confirmTitle();
    CHECK(!titleOn&&menuOn&&paused,"missing save medium keeps the world open");
    CHECK(stateHash()==before,"missing save medium preserves progress");
    CHECK(strstr(info,"NOT SAVED")&&savedYear==-1&&!hasSave,"missing medium reports failure without claiming a save");
    sdOk=1;savePath="missing/world.sav";confirmTitle();
    CHECK(!titleOn&&menuOn&&paused&&fxQ==FX_ERROR,"write failure returns to pause menu with feedback");
    CHECK(stateHash()==before&&savedYear==-1,"write failure preserves the current world");
    CHECK(!strcmp(info,"SAVE FAILED"),"write failure remains visible");

    size_t size=0;u8*data=readFile("fixture.sav",&size);if(!data)return 1;
    const size_t cuts[]={0,3,sizeof(SaveHdr)-1,sizeof(SaveHdr)+gW+17,
        sizeof(SaveHdr)+3u*gW*gH+nCr*sizeof(Cr)-1,size-1};
    for(size_t i=0;i<sizeof cuts/sizeof cuts[0];i++){
        CHECK(writeFile("broken.sav",data,cuts[i]),"write truncated fixture");
        CHECK(!loadGame("broken.sav"),"truncated save is rejected");
        CHECK(stateHash()==before,"truncated load leaves all current progress intact");
    }
    u8 first=data[0];data[0]=0;
    CHECK(writeFile("broken.sav",data,size)&&!loadGame("broken.sav"),"damaged header is rejected");
    CHECK(stateHash()==before,"damaged header preserves favorites and discoveries");data[0]=first;
    size_t countAt=size-sizeof rz-4;u32 oldCount;
    memcpy(&oldCount,data+countAt,4);memset(data+countAt,0xFF,4);
    CHECK(writeFile("broken.sav",data,size)&&!loadGame("broken.sav"),"invalid final trailer count is rejected");
    CHECK(stateHash()==before,"invalid trailer count leaves live state intact");memcpy(data+countAt,&oldCount,4);
    u32 identity=0x314E4747u;size_t idAt=0;
    for(size_t i=sizeof(SaveHdr);i+13<size;i++)if(!memcmp(data+i,&identity,4)){idAt=i;break;}
    CHECK(idAt!=0&&nCr>0,"locate identity records in real save");
    if(idAt){u8 gap=data[idAt+12];data[idAt+12]=0;
        CHECK(writeFile("broken.sav",data,size)&&!loadGame("broken.sav"),"invalid identity stream is rejected");
        CHECK(stateHash()==before,"invalid identity stream preserves the current world");data[idAt+12]=gap;}
    // Complete-but-corrupt archives must not reach occ[][], species,
    // village or kingdom lookups with indices from the damaged records.
    size_t creaturesAt=sizeof(SaveHdr)+3u*gW*gH;
    coord_t outside=(coord_t)-1;u16 unknownSpecies=(u16)nSp,unknownItem=(u16)(nIt+1);
    rejectRecordChange(data,size,creaturesAt+offsetof(Cr,x),&outside,sizeof outside,before,"invalid live creature x is rejected");
    rejectRecordChange(data,size,creaturesAt+offsetof(Cr,y),&outside,sizeof outside,before,"invalid live creature y is rejected");
    rejectRecordChange(data,size,creaturesAt+offsetof(Cr,sp),&unknownSpecies,sizeof unknownSpecies,before,"unknown live creature species is rejected");
    rejectRecordChange(data,size,creaturesAt+offsetof(Cr,wep),&unknownItem,sizeof unknownItem,before,"unknown equipped weapon is rejected");
    rejectRecordChange(data,size,creaturesAt+offsetof(Cr,arm),&unknownItem,sizeof unknownItem,before,"unknown equipped armor is rejected");
    u8 unknownTrait=(u8)(sizeof trNm/sizeof trNm[0]);
    rejectRecordChange(data,size,creaturesAt+offsetof(Cr,tr),&unknownTrait,1,before,"unknown primary trait is rejected");
    rejectRecordChange(data,size,creaturesAt+offsetof(Cr,tr2),&unknownTrait,1,before,"unknown secondary trait is rejected");
    int liveVillage=-1;for(int v=0;v<VIL_N;v++)if(vil[v].on){liveVillage=v;break;}
    CHECK(liveVillage>=0,"fixture contains an inhabited village");
    size_t villagesAt=creaturesAt+nCr*sizeof(Cr);
    if(liveVillage>=0){
        size_t at=villagesAt+(size_t)liveVillage*sizeof(Vil);u8 invalidRace=8,invalidFoe=255,invalidKing=KN_N+1;
        rejectRecordChange(data,size,at+offsetof(Vil,x),&outside,sizeof outside,before,"invalid village x is rejected");
        rejectRecordChange(data,size,at+offsetof(Vil,y),&outside,sizeof outside,before,"invalid village y is rejected");
        rejectRecordChange(data,size,at+offsetof(Vil,race),&invalidRace,1,before,"invalid village race is rejected");
        rejectRecordChange(data,size,at+offsetof(Vil,foe),&invalidFoe,1,before,"invalid village enemy is rejected");
        size_t kingsAt=villagesAt+VIL_N*sizeof(Vil);
        rejectRecordChange(data,size,kingsAt+sizeof kd+VIL_N+liveVillage,&invalidKing,1,before,"invalid village kingdom is rejected");
        Kd brokenKing={0};brokenKing.on=1;brokenKing.lord=KN_N+1;
        rejectRecordChange(data,size,kingsAt,&brokenKing,sizeof brokenKing,before,"invalid kingdom overlord is rejected");
        brokenKing.lord=0;brokenKing.tech=(u8)(sizeof techN/sizeof techN[0]);
        rejectRecordChange(data,size,kingsAt,&brokenKing,sizeof brokenKing,before,"invalid kingdom technology is rejected");
    }
#ifdef GG_BIGMAP
    // The normal cross-build suite covers a complete DS save widening.
    // Exercise the corresponding 8-bit record rejection path as well.
    SaveHdr legacy={0};legacy.gw=128;legacy.szCr=sizeof(CrOld);legacy.szVil=sizeof(VilOld);
    for(int mode=0;mode<6;mode++){
        CrOld old={0};old.alive=1;old.x=64;old.y=64;
        if(mode==1)old.x=255;if(mode==2)old.y=255;
        if(mode==3)old.sp=(u16)nSp;if(mode==4)old.wep=(u16)(nIt+1);if(mode==5)old.tr=unknownTrait;
        FILE*f=tmpfile();CHECK(f!=NULL,"open legacy-record fixture");
        if(f){CHECK(fwrite(&old,sizeof old,1,f)==1,"write legacy-record fixture");rewind(f);
            CHECK(saveCreatureRecordsValid(f,&legacy,1)==(mode==0),"legacy creature coordinate/species/item/trait validation");fclose(f);}
    }
    for(int mode=0;mode<4;mode++){
        VilOld old={0};old.on=1;old.x=64;old.y=64;
        if(mode==1)old.x=255;if(mode==2)old.race=8;if(mode==3)old.foe=255;
        FILE*f=tmpfile();CHECK(f!=NULL,"open legacy-village fixture");
        if(f){CHECK(fwrite(&old,sizeof old,1,f)==1,"write legacy-village fixture");rewind(f);
            CHECK(saveVillageRecordsValid(f,&legacy,1,1)==(mode==0),"legacy village coordinate/race/reference validation");fclose(f);}
    }
    CHECK(stateHash()==before,"legacy validation never mutates the current world");
#endif
    u8 invalidTile=T_N,invalidBuilding=B_N,invalidOwner=255;
    rejectRecordChange(data,size,sizeof(SaveHdr),&invalidTile,1,before,"invalid terrain tile is rejected");
    rejectRecordChange(data,size,sizeof(SaveHdr)+gW,&invalidBuilding,1,before,"invalid building tile is rejected");
    rejectRecordChange(data,size,sizeof(SaveHdr)+2u*gW,&invalidOwner,1,before,"invalid building owner is rejected");
    free(data);

    savePath=SAVE_PATH;CHECK(saveGame(savePath),"save slot first generation");
    year=22;CHECK(saveGame(savePath),"save slot second generation");
    CHECK(writeFile(savePath,"bad",3),"damage primary while preserving backup");
    slotScan();CHECK(slotYr[0]==21,"slot list shows the backup year for a corrupt primary");
    CHECK(loadGame(savePath)&&year==21,"backup is recoverable through ordinary load");
    year=27;u32 sourceSeed=worldSeed;confirmTitle();
    CHECK(titleOn&&paused&&hasSave&&savedYear==27,"successful save safely returns to title");
    CHECK(loadGame(savePath)&&year==27&&worldSeed==sourceSeed,"save-and-title preserves the outgoing world on disk");
    remove("fixture.sav");remove("broken.sav");remove(SAVE_PATH);remove(SAVE_PATH ".bak");
    chdir("/tmp");rmdir(temp);
    printf(fails?"SAVE SAFETY FAILED (%d)\n":"SAVE SAFETY OK (%d failures)\n",fails);
    return fails?1:0;
}
