// SDK-free fixture for the actual Unwritten Reign expedition runtime.
#define GG_TEST_NATIVE_ORIGIN
#define GG_CHRON_SAVE_PATH "chronicles.dat"
#define GG_CHRON_CHECKPOINT_PATH "expedition.dat"
#define GG_REIGN_SAVE_PATH "reign.dat"
#define GG_SAGA_SAVE_PATH "saga.dat"
#include <sys/stat.h>
#include "3ds/main3ds.c"

static void require(int condition,const char *reason){
    if(!condition){fprintf(stderr,"CHRONICLE CHECKPOINT FAIL: %s\n",reason);exit(1);}
}

static void clearRuntime(void){
    memset(&ggChron,0,sizeof ggChron);
    memset(&ggExplore,0,sizeof ggExplore);
    memset(&ggChronSessionCheckpoint,0,sizeof ggChronSessionCheckpoint);
}

static void fileWrite(const char *path,const void *data,size_t length){
    FILE *f=fopen(path,"wb");
    require(f&&fwrite(data,length,1,f)==1&&!fclose(f),"wrote fixture record");
}

int main(void){
    char dir[]="/tmp/gg_chronicle_checkpoint_XXXXXX";
    require(mkdtemp(dir)&&!chdir(dir),"isolated save directory");
    memset(world,T_GRASS,sizeof world);
    chapter=14;worldSeed=135791u;gW=gH=128;
    require(ggChronBegin(45,45,0)>0,"fresh expedition entered");
    require(ggChronCheckpoint(),"saved a checkpoint during the opening");
    clearRuntime();
    require(ggChronBegin(90,90,0)>0&&ggChron.resume==GG_CHRON_RESUME_OPEN,
            "an unfinished opening remained pending");
    ggChronFinishOpening();
    clearRuntime();
    require(ggChronBegin(90,90,0)>0&&ggChron.resume==GG_CHRON_RESUME_FIELD,
            "a completed opening stayed complete");
    ggChron.briefTime=0;
    ggExplore.px=ggChron.px[0]*256+128;
    ggExplore.py=ggChron.py[0]*256+128;
    require(!ggChronStep(KEY_X)&&ggChron.post==1,"first post secured");
    short postX=ggChron.px[1],postY=ggChron.py[1];
    int ox=ggChron.routeOx,oy=ggChron.routeOy;
    int injured=2;
    require(ggChron.foe[injured].post==1,"second post has guards");
    clearRuntime();
    require(ggChronBegin(90,90,0)>0&&ggChron.post==1,
            "securing a post checkpointed even without a map exit");

    ggChron.foe[injured].hp=7;
    ggChron.hp=61;ggChron.stamina=32;ggChron.focus=18;
    ggChron.food=24;ggChron.rations=1;ggChron.morale=43;
    ggChron.style=GG_CHRON_EMBER;ggChron.briefTime=0;
    ggExplore.angle=194;ggExplore.px+=27;
    int px=ggExplore.px,py=ggExplore.py,frame=ggChron.frame;
    require(ggChronStep(KEY_START)==1,"field exit returned to map");
    clearRuntime();
    require(ggChronBegin(90,90,1)>0,"relaunch entered");
    require(ggChron.post==1&&ggChron.px[1]==postX&&ggChron.py[1]==postY&&
            ggChron.routeOx==ox&&ggChron.routeOy==oy,"secured post and route survived relaunch");
    require(ggChron.foe[injured].hp==7&&ggChronEnemiesAt(0)==0,"injured and surrendered guards survived relaunch");
    require(ggChron.hp==61&&ggChron.stamina==32&&ggChron.focus==18&&
            ggChron.food==24&&ggChron.rations==1&&ggChron.morale==43,
            "leaving did not restore supplies or health");
    require(ggChron.style==GG_CHRON_EMBER&&ggChron.frame==frame&&
            ggExplore.px==px&&ggExplore.py==py&&ggExplore.angle==194&&ggExplore.wasPaused,
            "relaunch restored walking position, heading, weapon and elapsed time");
    int nx,ny,steps;
    require(ggChronNavNext(px>>8,py>>8,&nx,&ny,&steps)&&steps>0,
            "relaunch rebuilt directions for the next post");
    require(!rename(GG_CHRON_CHECKPOINT_PATH,GG_CHRON_CHECKPOINT_PATH ".bak"),
            "simulated interruption after creating a backup");
    clearRuntime();
    require(ggChronBegin(90,90,0)>0&&ggChron.post==1&&ggChron.hp==61,
            "a committed backup survived interrupted replacement");
    require(!rename(GG_CHRON_CHECKPOINT_PATH ".bak",GG_CHRON_CHECKPOINT_PATH),
            "restored fixture's primary checkpoint");

    clearRuntime();worldSeed++;
    require(ggChronBegin(90,90,0)>0&&ggChron.post==0&&ggChron.hp==100&&
            ggChron.routeOx!=ox,"same act on another seed ignored the checkpoint");
    clearRuntime();worldSeed--;gW=gH=192;
    require(ggChronBegin(90,90,0)>0&&ggChron.post==0&&ggChron.routeOx!=ox,
            "same seed on another planet size ignored the checkpoint");

    clearRuntime();gW=gH=128;
    world[postY][postX]=T_WATER;
    require(ggChronBegin(90,90,0)>0&&ggChron.post==0&&ggChron.routeOx!=ox,
            "an impassable saved objective was rejected");
    world[postY][postX]=T_GRASS;
    clearRuntime();
    require(ggChronBegin(90,90,0)>0&&ggChron.post==1&&ggChron.hp==61,
            "same-world checkpoint survived incompatible loads");
    world[py>>8][px>>8]=T_WATER;
    clearRuntime();
    require(ggChronBegin(90,90,0)>0&&ggChron.relocated&&ggChron.post==1&&ggChron.hp==61&&
            ggChronTerrain(ggExplore.px>>8,ggExplore.py>>8),
            "a flooded standing area relocated the commander and kept supplies");
    world[py>>8][px>>8]=T_GRASS;

    // A write error should preserve the latest state for a same-session map return.
    require(!mkdir(GG_CHRON_CHECKPOINT_PATH ".tmp",0700),"blocked checkpoint overwrite");
    ggChron.hp=46;ggChron.food=14;
    require(ggChronStep(KEY_START)&&!ggChron.checkpointWritten,"reported the write error");
    memset(&ggChron,0,sizeof ggChron);memset(&ggExplore,0,sizeof ggExplore);
    require(ggChronBegin(90,90,0)>0&&ggChron.hp==46&&ggChron.food==14,
            "latest checkpoint survived write failure in the same session");
    require(!rmdir(GG_CHRON_CHECKPOINT_PATH ".tmp"),"released blocked overwrite");

    ggChron.hp=0;ggChron.rations=2;ggChron.morale=43;
    ggChron.food=14;ggChron.focus=18;ggChron.stamina=32;
    require(!ggChronStep(0)&&ggChron.lost,"defeat checkpointed immediately");
    clearRuntime();
    require(ggChronBegin(90,90,0)>0&&ggChron.resume==GG_CHRON_RESUME_DEFEAT&&
            ggChron.relocated&&!ggChron.lost&&ggChron.hp==35&&ggChron.post==1&&
            ggChron.rations==1&&ggChron.morale==31&&ggChron.food==14&&
            ggChron.focus==18&&ggChron.foe[injured].hp==7,"defeat paid the camp recovery cost and preserved progress");

    ggChron.post=GG_CHRON_POSTS;
    for(int i=0;i<ggChron.foeN;i++)ggChron.foe[i].hp=0;
    require(ggChronCheckpoint(),"saved the final decision checkpoint");
    clearRuntime();
    require(ggChronBegin(90,90,0)>0&&ggChron.resume==GG_CHRON_RESUME_CHOICE,
            "a pending act decision survived a restart");

    ggReignBegin();ggSagaStartChoice();
    int crowns=ggReign.credits,score=ggChron.score;
    require(!mkdir(GG_CHRON_SAVE_PATH,0700),"blocked the completed-act file");
    ggSagaCommitChoice(1);
    require(!ggChron.won&&ggChron.cleared==0&&ggSaga.mode==GG_SAGA_CHOICE&&
            ggSaga.saveFailed&&!(ggSaga.decided&1)&&ggReign.credits==crowns&&ggChron.score==score,
            "failed completion did not grant repeatable rewards or leave the choice");
    require(ggChronCheckpoint(),"leaving saved the pending choice separately");
    clearRuntime();memset(&ggSaga,0,sizeof ggSaga);memset(&ggReign,0,sizeof ggReign);
    ggExpEnterReign(0);
    require(ggChron.resume==GG_CHRON_RESUME_CHOICE&&ggSaga.mode==GG_SAGA_CHOICE&&
            ggReign.credits==crowns&&ggChron.score==score,
            "restart reopened the decision without any failed-write reward");
    require(!rmdir(GG_CHRON_SAVE_PATH),"released the blocked completed-act file");
    ggSagaCommitChoice(1);
    require(ggChron.won&&ggChron.cleared==1&&ggSaga.mode==GG_SAGA_OUTRO&&
            (ggSaga.decided&1)&&ggReign.credits==crowns+18,
            "completion retry granted its reward once");

    // Earlier releases wrote completed acts and their decisions independently.
    const u32 clearV1[]={0x47474348u,1,1,0x47474348u^1u^1u^0x5aa59669u};
    GgSagaDisk sagaV1={0x47535341u,1,1,1,0,0};
    sagaV1.checksum=ggSagaChecksum(&sagaV1);
    fileWrite(GG_CHRON_SAVE_PATH,clearV1,sizeof clearV1);
    fileWrite(GG_SAGA_SAVE_PATH,&sagaV1,sizeof sagaV1);
    remove(GG_REIGN_SAVE_PATH);
    clearRuntime();memset(&ggSaga,0,sizeof ggSaga);memset(&ggReign,0,sizeof ggReign);
    require(ggChronBegin(90,90,0)>0&&ggChron.act==1&&ggChron.post==0,
            "legacy completion advanced the act and excluded the old checkpoint");
    ggSagaBegin();
    require(ggSaga.decisions==1&&ggChron.rations==4&&ggChron.morale==61,
            "legacy decision remained readable and gave its carryover once");

    ggChronFinishOpening();
    ggReignBegin();
    int campX=ggExplore.px>>8,campY=ggExplore.py>>8;
    vil[0].on=1;vil[0].x=campX;vil[0].y=campY;vil[0].stock=25;
    require(ggReignRecruit()&&ggReign.credits==20&&vil[0].stock==20,
            "recruit paid real campaign and town costs");
    clearRuntime();memset(&ggReign,0,sizeof ggReign);
    ggExpEnterReign(0);
    require(ggChron.active&&ggChron.resume==GG_CHRON_RESUME_FIELD&&ggSaga.mode==GG_SAGA_IDLE&&
            ggChron.rations==4&&ggChron.morale==61,
            "native relaunch kept the opening complete and prior-act bonus applied once");
    require(ggReign.squadN==1&&ggReign.credits==20,"paid recruit stayed with the expedition");
    ggReign.squad[0].hp=24;ggReign.order=GG_REIGN_HOLD;
    ggReign.frame=89;ggReign.nextCivicAt=142;
    uint32_t identity=ggReign.squadArt[0].identity;
    int guardX=ggReign.squad[0].x,guardY=ggReign.squad[0].y;
    require(ggChronStep(KEY_START),"squad departed to the map");
    clearRuntime();memset(&ggReign,0,sizeof ggReign);
    world[guardY][guardX]=T_WATER;
    require(ggChronBegin(80,80,0)>0,"reopened with flooded guard position");
    ggReignBegin();ggChronRestoreSquad();
    require(ggReign.squadN==1&&ggReign.squad[0].hp==24&&ggReign.order==GG_REIGN_HOLD&&
            ggReign.squadArt[0].identity==identity&&ggReign.credits==20&&
            ggReign.frame==89&&ggReign.nextCivicAt==142&&
            ggChronLand(ggReign.squad[0].x,ggReign.squad[0].y),
            "injured recruit rallied on safe terrain with the same order and identity");
    world[guardY][guardX]=T_GRASS;

    clearRuntime();worldSeed++;
    occ[45][45]=1;
    require(ggChronBegin(45,45,0)>0&&ggChronLand(ggExplore.px>>8,ggExplore.py>>8),
            "fresh expedition from a resident chose an unoccupied starting tile");

    remove(GG_CHRON_CHECKPOINT_PATH);remove(GG_CHRON_SAVE_PATH);
    remove(GG_SAGA_SAVE_PATH);remove(GG_REIGN_SAVE_PATH);
    require(!chdir("/tmp")&&!rmdir(dir),"removed fixture files");
    puts("CHRONICLE CHECKPOINT OK: active acts, squads, supplies, defeat, navigation and world validation");
    return 0;
}
