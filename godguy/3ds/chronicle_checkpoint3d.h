// One fixed-size expedition checkpoint, independent of story and world saves.
#ifndef GG_CHRONICLE_CHECKPOINT3D_H
#define GG_CHRONICLE_CHECKPOINT3D_H

#include <stdint.h>
#ifndef GG_CHRON_CHECKPOINT_PATH
#define GG_CHRON_CHECKPOINT_PATH "sdmc:/godguy_expedition.dat"
#endif

typedef struct { int32_t x,y,hp,post,wait,alert; } GgChronSavedFoe;
typedef struct { int32_t x,y,hp,heading; uint32_t identity; } GgChronSavedGuard;
typedef struct {
    uint32_t magic,version,seed;
    int32_t width,height,act,cleared;
    int32_t opening,defeated,post,frame,briefTime;
    int32_t hp,stamina,food,rations,morale,focus,score,style,attackWait;
    int32_t routeOx,routeOy,x,y,angle;
    int32_t postX[GG_CHRON_POSTS],postY[GG_CHRON_POSTS],foeN;
    GgChronSavedFoe foe[GG_CHRON_FOES];
    int32_t squadN,squadOrder,reignFrame,civicAt;
    GgChronSavedGuard squad[GG_CHRON_SQUAD_MAX];
    uint32_t checksum;
} GgChronCheckpoint;
_Static_assert(sizeof(GgChronCheckpoint)==488,"Fixed expedition save layout");
static void ggChronCaptureSquad(GgChronCheckpoint *c);

// A map exit remains resumable in this session even if the SD write fails.
static GgChronCheckpoint ggChronSessionCheckpoint;

static uint32_t ggChronCheckpointHash(const GgChronCheckpoint *c){
    const unsigned char *p=(const unsigned char *)c;
    uint32_t h=2166136261u;
    for(unsigned i=0;i<sizeof *c-sizeof c->checksum;i++)h=(h^p[i])*16777619u;
    return h;
}
static int ggChronCheckpointKey(const GgChronCheckpoint *c,int act,int cleared){
    return c->magic==0x47474558u&&c->version==1&&c->seed==worldSeed&&
           c->width==WW&&c->height==WH&&c->act==act&&c->cleared==cleared;
}
static int ggChronReadCheckpointFile(const char *path,GgChronCheckpoint *c){
    FILE *f=fopen(path,"rb");
    if(!f)return 0;
    int good=fread(c,sizeof *c,1,f)==1&&fgetc(f)==EOF&&!ferror(f);
    fclose(f);
    return good&&c->magic==0x47474558u&&c->version==1&&
           c->checksum==ggChronCheckpointHash(c)?1:-1;
}
static int ggChronReadCheckpoint(GgChronCheckpoint *c){
    if(ggChronCheckpointKey(&ggChronSessionCheckpoint,ggChron.act,ggChron.cleared)){
        *c=ggChronSessionCheckpoint;
        return 1;
    }
    int read=ggChronReadCheckpointFile(GG_CHRON_CHECKPOINT_PATH,c);
    if(read<=0){
        int backup=ggChronReadCheckpointFile(GG_CHRON_CHECKPOINT_PATH ".bak",c);
        if(backup)read=backup;
    }
    if(read<=0)return read;
    return ggChronCheckpointKey(c,ggChron.act,ggChron.cleared);
}
static int ggChronWriteRecord(const char *path,const char *tmp,const char *bak,
                              const void *record,size_t size){
    FILE *f=fopen(tmp,"wb");
    if(!f)return 0;
    int good=fwrite(record,size,1,f)==1;
    if(fclose(f)||!good){remove(tmp);return 0;}
    if(rename(tmp,path)){
        // FAT may require the destination to be absent before a rename.
        FILE *prior=fopen(path,"rb");
        if(!prior){remove(tmp);return 0;}
        (void)fgetc(prior);
        int readable=!ferror(prior);
        fclose(prior);
        if(!readable){remove(tmp);return 0;}
        remove(bak);
        if(rename(path,bak)){remove(tmp);return 0;}
        if(rename(tmp,path)){rename(bak,path);remove(tmp);return 0;}
    }
    remove(bak);
    return 1;
}
static int ggChronCheckpoint(void){
    if(!ggChron.active)return 1;
    if(ggChron.won)return ggChron.checkpointWritten;
    GgChronCheckpoint c={
        .magic=0x47474558u,.version=1,.seed=worldSeed,.width=WW,.height=WH,
        .act=ggChron.act,.cleared=ggChron.cleared,.opening=ggChron.openingPending,
        .defeated=ggChron.lost,.post=ggChron.post,.frame=ggChron.frame,
        .briefTime=ggChron.briefTime,.hp=ggChron.hp,.stamina=ggChron.stamina,
        .food=ggChron.food,.rations=ggChron.rations,.morale=ggChron.morale,
        .focus=ggChron.focus,.score=ggChron.score,.style=ggChron.style,
        .attackWait=ggChron.attackWait,.routeOx=ggChron.routeOx,.routeOy=ggChron.routeOy,
        .x=ggExplore.px,.y=ggExplore.py,.angle=ggExplore.angle,.foeN=ggChron.foeN
    };
    for(int p=0;p<GG_CHRON_POSTS;p++){
        c.postX[p]=ggChron.px[p];c.postY[p]=ggChron.py[p];
    }
    for(int i=0;i<ggChron.foeN;i++){
        const GgChronFoe *f=&ggChron.foe[i];
        c.foe[i]=(GgChronSavedFoe){f->x,f->y,f->hp>0?f->hp:0,
                                  f->post,f->wait,ggChron.foeAlert[i]};
    }
    ggChronCaptureSquad(&c);
    c.checksum=ggChronCheckpointHash(&c);
    ggChronSessionCheckpoint=c;
    ggChron.checkpointWritten=ggChronWriteRecord(GG_CHRON_CHECKPOINT_PATH,
        GG_CHRON_CHECKPOINT_PATH ".tmp",GG_CHRON_CHECKPOINT_PATH ".bak",&c,sizeof c);
    return ggChron.checkpointWritten;
}
static void ggChronFinishOpening(void){
    if(ggChron.openingPending){
        ggChron.openingPending=0;
        ggChronCheckpoint();
    }
}
static void ggChronClearCompletedCheckpoint(void){
    GgChronCheckpoint disk;
    if(ggChronCheckpointKey(&ggChronSessionCheckpoint,ggChron.act,GG_CHRON_ACTS))
        ggChronSessionCheckpoint=(GgChronCheckpoint){0};
    // Act advancement rejects its predecessor by key. Clear a completed
    // final-act replay too, since its unlocked act number stays the same.
    if(ggChronReadCheckpointFile(GG_CHRON_CHECKPOINT_PATH,&disk)>0&&
       ggChronCheckpointKey(&disk,ggChron.act,GG_CHRON_ACTS))
        remove(GG_CHRON_CHECKPOINT_PATH);
    if(ggChronReadCheckpointFile(GG_CHRON_CHECKPOINT_PATH ".bak",&disk)>0&&
       ggChronCheckpointKey(&disk,ggChron.act,GG_CHRON_ACTS))
        remove(GG_CHRON_CHECKPOINT_PATH ".bak");
}

static int ggChronCheckpointShape(const GgChronCheckpoint *c){
    int mid=GG_CHRON_GRID/2;
    if(c->routeOx<1-mid||c->routeOx>=WW-1-mid||
       c->routeOy<1-mid||c->routeOy>=WH-1-mid||
       c->x<256||c->x>=(WW-1)*256||c->y<256||c->y>=(WH-1)*256||
       (unsigned)c->angle>255||(unsigned)c->post>GG_CHRON_POSTS||
       (unsigned)c->opening>1||(unsigned)c->defeated>1||
       (c->opening&&(c->post||c->frame||c->defeated))||
       (c->defeated!=(c->hp==0))||(c->defeated&&c->post==GG_CHRON_POSTS)||
       (unsigned)c->frame>0x3fffffffu||(unsigned)c->briefTime>165||
       (unsigned)c->hp>100||(unsigned)c->stamina>100||(unsigned)c->food>100||
       (unsigned)c->rations>9||(unsigned)c->morale>100||(unsigned)c->focus>100||
       (unsigned)c->score>1000000||(unsigned)c->style>=GG_CHRON_STYLES||
       (unsigned)c->attackWait>36||(unsigned)c->foeN>GG_CHRON_FOES||
       (unsigned)c->squadN>GG_CHRON_SQUAD_MAX||(unsigned)c->squadOrder>2||
       (unsigned)c->reignFrame>0x3fffffffu||
       (unsigned)c->civicAt>(unsigned)c->reignFrame+95u)return 0;
    for(int p=0;p<GG_CHRON_POSTS;p++){
        int x=c->postX[p],y=c->postY[p];
        if(x<1||x>=WW-1||y<1||y>=WH-1||x<c->routeOx||
           x>=c->routeOx+GG_CHRON_GRID||y<c->routeOy||y>=c->routeOy+GG_CHRON_GRID||
           (p>=c->post&&!ggChronTerrain(x,y)))return 0;
        for(int j=0;j<p;j++)
            if(ggChronNear(x,y,c->postX[j],c->postY[j])<5)return 0;
    }
    for(int i=0;i<c->foeN;i++){
        const GgChronSavedFoe *f=&c->foe[i];
        if(f->x<1||f->x>=WW-1||f->y<1||f->y>=WH-1||
           (unsigned)f->hp>105||(unsigned)f->post>=GG_CHRON_POSTS||
           (unsigned)f->wait>127||(unsigned)f->alert>60)return 0;
        if(!f->hp)continue;
        if(f->post<c->post||!ggChronTerrain(f->x,f->y))return 0;
        for(int j=0;j<i;j++)
            if(c->foe[j].hp&&c->foe[j].x==f->x&&c->foe[j].y==f->y)return 0;
    }
    for(int i=0;i<c->squadN;i++){
        const GgChronSavedGuard *g=&c->squad[i];
        if((g->x||g->y)&&(g->x<1||g->x>=WW-1||g->y<1||g->y>=WH-1))return 0;
        if((unsigned)g->hp>45||(unsigned)g->heading>255)return 0;
    }
    return 1;
}
static int ggChronCheckpointDanger(const GgChronCheckpoint *c,int x,int y){
    int danger=0;
    for(int i=0;i<c->foeN;i++){
        const GgChronSavedFoe *f=&c->foe[i];
        if(!f->hp)continue;
        int d=ggChronNear(x,y,f->x,f->y);
        if(d==0)return -1;
        if(d<3)danger+=3-d;
    }
    return danger;
}
static int ggChronRestoreCheckpoint(const GgChronCheckpoint *c,int oldPause){
    if(!ggChronCheckpointShape(c))return 0;
    ggChron.routeOx=c->routeOx;ggChron.routeOy=c->routeOy;
    for(int p=0;p<GG_CHRON_POSTS;p++){
        ggChron.px[p]=(short)c->postX[p];ggChron.py[p]=(short)c->postY[p];
    }
    // Future objectives must be reachable in sequence, including one-way
    // slopes and gate orientation. Leave the current objective's flow live.
    int next=c->post<GG_CHRON_POSTS?c->post:GG_CHRON_POSTS-1;
    if(!ggChronTerrain(c->postX[next],c->postY[next]))return 0;
    for(int p=GG_CHRON_POSTS-1;p>=next;p--){
        ggChronNavBuild(p);
        if(p>next){
            int from=ggChronRouteIndex(c->postX[p-1],c->postY[p-1]);
            if(from<0||ggChronNavDistance[from]==255)return 0;
        }
    }
    int x=c->x>>8,y=c->y>>8,q=ggChronRouteIndex(x,y);
    int px=c->x,py=c->y;
    if(c->defeated||!ggChronLand(x,y)||ggChronCheckpointDanger(c,x,y)<0||
       (c->post<GG_CHRON_POSTS&&(q<0||ggChronNavDistance[q]==255))){
        // Rally on the same route when the saved tile was cut off or the
        // commander fell. Prefer open ground away from live guards.
        int anchorX=c->routeOx+GG_CHRON_GRID/2;
        int anchorY=c->routeOy+GG_CHRON_GRID/2;
        if(c->post){anchorX=c->postX[c->post-1];anchorY=c->postY[c->post-1];}
        int best=-1,bestCost=0x7fffffff;
        for(int i=0;i<GG_CHRON_CELLS;i++){
            if(ggChronNavDistance[i]==255)continue;
            int tx=c->routeOx+i%GG_CHRON_GRID,ty=c->routeOy+i/GG_CHRON_GRID;
            if(occ[ty][tx])continue;
            int danger=ggChronCheckpointDanger(c,tx,ty);
            if(danger<0)continue;
            int cost=ggChronNear(tx,ty,anchorX,anchorY)+danger*GG_CHRON_GRID;
            if(cost<bestCost){best=i;bestCost=cost;}
        }
        if(best<0)return 0;
        x=c->routeOx+best%GG_CHRON_GRID;y=c->routeOy+best/GG_CHRON_GRID;
        px=x*256+128;py=y*256+128;
        ggChron.relocated=1;
    }
    if(!ggExpBegin(x,y,oldPause))return 0;
    ggExplore.px=px;ggExplore.py=py;ggExplore.angle=c->angle;
    ggExplore.frame=c->frame;
    ggChron.openingPending=c->opening;ggChron.post=c->post;
    ggChron.frame=c->frame;ggChron.day=(c->frame/800)%4;
    ggChron.briefTime=c->briefTime;ggChron.attackWait=c->attackWait;
    ggChron.hp=c->hp;ggChron.stamina=c->stamina;ggChron.food=c->food;
    ggChron.rations=c->rations;ggChron.morale=c->morale;ggChron.focus=c->focus;
    ggChron.score=c->score;ggChron.style=c->style;ggChron.foeN=c->foeN;
    for(int i=0;i<c->foeN;i++){
        const GgChronSavedFoe *f=&c->foe[i];
        ggChron.foe[i]=(GgChronFoe){(short)f->x,(short)f->y,(signed char)f->hp,
                                    (signed char)f->post,(signed char)f->wait};
        ggChron.foeAlert[i]=(unsigned char)f->alert;
    }
    ggChron.resume=c->opening?GG_CHRON_RESUME_OPEN:c->post==GG_CHRON_POSTS?
                   GG_CHRON_RESUME_CHOICE:GG_CHRON_RESUME_FIELD;
    if(c->defeated){
        ggChron.resume=GG_CHRON_RESUME_DEFEAT;
        ggChron.hp=35;
        ggChron.morale=ggExpClamp(ggChron.morale-12,0,100);
        if(ggChron.rations)ggChron.rations--;
        ggChron.briefTime=90;
    }
    if(c->post==GG_CHRON_POSTS)ggChronNavBuild(c->post);
    ggChronSessionCheckpoint=*c;
    return 1;
}
#endif
