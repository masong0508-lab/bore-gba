// GodGuy: The Unwritten Reign - 3DS original opening and character creator.
// The playable character is born ordinary to everyone who knows them.
// Their dormant demigod nature is revealed at age 22, not at birth.
// The opening uses projected scenery and the original character cels.
// Uses the existing 400x240 top buffer; no heap or extra framebuffer.
#ifndef GG_ORIGIN3D_H
#define GG_ORIGIN3D_H
#ifndef GG_ORIGIN_SAVE_PATH
#define GG_ORIGIN_SAVE_PATH "sdmc:/godguy_origin.dat"
#endif
enum { GG_ORIGIN_BIRTH,GG_ORIGIN_CHILD,GG_ORIGIN_YOUTH,
       GG_ORIGIN_CREATE,GG_ORIGIN_AWAKEN,GG_ORIGIN_EPILOGUE,
       GG_ORIGIN_FINISHED };
enum { GG_ORIGIN_NAMES=8, GG_ORIGIN_SKINS=6, GG_ORIGIN_HAIRS=6,
       GG_ORIGIN_GARMENTS=6,GG_ORIGIN_HOMES=4,GG_ORIGIN_BODIES=3 };
typedef struct {
    int loaded,complete,active,phase,frame,lastKeys;
    int row,name,skin,hair,garment,home,body,flash,signal;
} GgOrigin;
static GgOrigin ggOrigin;
static const char *const ggOriginNames[GG_ORIGIN_NAMES]={
    "ARIN","SABLE","ROWAN","LARK","THALE","NOVA","FINN","EMBER"
};
static const char *const ggOriginHomes[GG_ORIGIN_HOMES]={
    "FIELDS","COAST","MARKET","HIGHLANDS"
};
static const char *const ggOriginRows[6]={
    "NAME","BUILD","COMPLEXION","HAIR","CLOTHES","UPBRINGING"
};
static const u8 ggOriginSkinRGB[GG_ORIGIN_SKINS][3]={
    {242,198,152},{201,149,112},{151,105,75},
    {107,72,58},{224,169,139},{177,125,96}
};
static const u8 ggOriginHairRGB[GG_ORIGIN_HAIRS][3]={
    {57,40,38},{156,93,49},{229,193,120},
    {96,65,121},{81,91,95},{174,58,62}
};
static const u8 ggOriginClothesRGB[GG_ORIGIN_GARMENTS][3]={
    {57,96,108},{117,73,88},{128,100,62},
    {55,111,86},{101,86,141},{144,70,53}
};
typedef struct {
    u32 magic,version,complete;
    u32 name,skin,hair,garment,home,body;
    u32 checksum;
} GgOriginDisk;
static u32 ggOriginChecksum(const GgOriginDisk *d){
    const u8 *p=(const u8 *)d;
    u32 h=2166136261u;
    for(unsigned i=0;i<sizeof(*d)-sizeof(d->checksum);i++)
        h=(h^p[i])*16777619u;
    return h;
}
static void ggOriginLoad(void){
    if(ggOrigin.loaded)return;
    ggOrigin.loaded=1;
    FILE *f=fopen(GG_ORIGIN_SAVE_PATH,"rb");
    if(!f)return;
    GgOriginDisk d;
    memset(&d,0,sizeof d);
    int ok=fread(&d,sizeof d,1,f)==1;
    fclose(f);
    if(!ok||d.magic!=0x47474f52u||d.version!=1||
       d.checksum!=ggOriginChecksum(&d)||!d.complete||
       d.name>=GG_ORIGIN_NAMES||d.skin>=GG_ORIGIN_SKINS||
       d.hair>=GG_ORIGIN_HAIRS||d.garment>=GG_ORIGIN_GARMENTS||
       d.home>=GG_ORIGIN_HOMES||d.body>=GG_ORIGIN_BODIES)return;
    ggOrigin.complete=1;
    ggOrigin.name=(int)d.name;ggOrigin.skin=(int)d.skin;
    ggOrigin.hair=(int)d.hair;ggOrigin.garment=(int)d.garment;
    ggOrigin.home=(int)d.home;ggOrigin.body=(int)d.body;
}
static void ggOriginSave(void){
    if(!ggOrigin.complete)return;
    GgOriginDisk d;
    memset(&d,0,sizeof d);
    d.magic=0x47474f52u;d.version=1;d.complete=1;
    d.name=(u32)ggOrigin.name;d.skin=(u32)ggOrigin.skin;
    d.hair=(u32)ggOrigin.hair;d.garment=(u32)ggOrigin.garment;
    d.home=(u32)ggOrigin.home;d.body=(u32)ggOrigin.body;
    d.checksum=ggOriginChecksum(&d);
    FILE *f=fopen(GG_ORIGIN_SAVE_PATH,"wb");
    if(!f)return;
    (void)fwrite(&d,sizeof d,1,f);
    fclose(f);
}
static void ggOriginBegin(void){
    ggOriginLoad();
    ggOrigin.active=!ggOrigin.complete;
    ggOrigin.phase=GG_ORIGIN_BIRTH;
    ggOrigin.frame=ggOrigin.lastKeys=ggOrigin.row=ggOrigin.flash=0;
}
static void ggOriginAdvance(void){
    ggOrigin.phase++;
    ggOrigin.frame=0;
    if(ggOrigin.phase>=GG_ORIGIN_FINISHED){
        ggOrigin.complete=1;ggOrigin.active=0;
        // The revelation unlocks a small, imperfect godborn ability,
        // rather than making the protagonist omnipotent from infancy.
        ggReign.divine=28;
        ggReignNotice("AGE 22: YOUR BLOOD IS NOT ENTIRELY MORTAL.");
        ggOriginSave();ggReignSave();
    }
}
static int ggOriginChoice(int row){
    switch(row){
    case 0:return ggOrigin.name;
    case 1:return ggOrigin.body;
    case 2:return ggOrigin.skin;
    case 3:return ggOrigin.hair;
    case 4:return ggOrigin.garment;
    default:return ggOrigin.home;
    }
}
static int ggOriginCount(int row){
    static const u8 counts[6]={
        GG_ORIGIN_NAMES,GG_ORIGIN_BODIES,GG_ORIGIN_SKINS,
        GG_ORIGIN_HAIRS,GG_ORIGIN_GARMENTS,GG_ORIGIN_HOMES
    };
    return counts[row];
}
static void ggOriginChoose(int row,int delta){
    int *value=&ggOrigin.name;
    switch(row){
    case 1:value=&ggOrigin.body;break;
    case 2:value=&ggOrigin.skin;break;
    case 3:value=&ggOrigin.hair;break;
    case 4:value=&ggOrigin.garment;break;
    case 5:value=&ggOrigin.home;break;
    }
    int count=ggOriginCount(row);
    *value=(*value+delta+count)%count;
}
// Return 1 only when the player explicitly retreats to the 2D story.
// The introduction is resumable (replays if unfinished), not a soft lock.
static int ggOriginStep(int raw){
    if(!ggOrigin.active)return 0;
    const int down=raw&~ggOrigin.lastKeys;
    ggOrigin.lastKeys=raw;
    ggOrigin.frame++;
    if((down&KEY_SELECT)&&(raw&KEY_START)){
        ggOrigin.active=0;return 1;
    }
    if(ggOrigin.phase==GG_ORIGIN_CREATE){
        if(down&KEY_UP)ggOrigin.row=(ggOrigin.row+5)%6;
        if(down&KEY_DOWN)ggOrigin.row=(ggOrigin.row+1)%6;
        if(down&KEY_LEFT)ggOriginChoose(ggOrigin.row,-1);
        if(down&KEY_RIGHT)ggOriginChoose(ggOrigin.row,1);
        if(down&(KEY_A|KEY_START))ggOriginAdvance();
    }else if(ggOrigin.phase<GG_ORIGIN_CREATE){
        if(down&KEY_START){
            ggOrigin.phase=GG_ORIGIN_CREATE;ggOrigin.frame=0;
        }else if((down&KEY_A)||ggOrigin.frame>=120)
            ggOriginAdvance();
    }else if(ggOrigin.phase==GG_ORIGIN_AWAKEN){
        if((down&KEY_A&&ggOrigin.frame>=40)||ggOrigin.frame>=220)
            ggOriginAdvance();
    }else if(ggOrigin.phase==GG_ORIGIN_EPILOGUE){
        if((down&(KEY_A|KEY_START))||ggOrigin.frame>=165)
            ggOriginAdvance();
    }
    return 0;
}
// The playable demigod is an original hand-authored 16x24 portrait cel
// (not a cube or block-headed avatar). Player palettes and body proportions
// change the actual animated character, with no new 3DS image buffers.
static const char *const ggOriginHumanCel[24]={
    "......hhhh......",
    ".....hhhhhh.....",
    "....hhhhhhhh....",
    "....hssssssh....",
    "....hsessesh....",
    "....hsissish....",
    "....hssssssh....",
    ".....ssssss.....",
    "......ssss......",
    "....dddccddd....",
    "...ddccccccdd...",
    "..ssddccccddss..",
    "..ssddaaaaddss..",
    "...sddccccdds...",
    "....dccccccd....",
    "....dcmccmcd....",
    "....ddccccdd....",
    ".....pppppp.....",
    ".....pp..pp.....",
    ".....pp..pp.....",
    ".....pp..pp.....",
    ".....pp..pp.....",
    ".....bb..bb.....",
    "....bbb..bbb...."
};
static void ggOriginPortrait(u8 *out,int x,int y,int scale,int frame,int stranger){
    const u8 *skin=ggOriginSkinRGB[(ggOrigin.skin+stranger)%GG_ORIGIN_SKINS];
    const u8 *hair=ggOriginHairRGB[(ggOrigin.hair+stranger*3)%GG_ORIGIN_HAIRS];
    const u8 *coat=ggOriginClothesRGB[(ggOrigin.garment+stranger*2)%GG_ORIGIN_GARMENTS];
    for(int py=0;py<24;py++){
        const char *row=ggOriginHumanCel[py];
        for(int px=0;px<16;px++){
            int kind=row[px],r=0,g=0,b=0;
            if(kind=='.')continue;
            switch(kind){
            case 'h':r=hair[0];g=hair[1];b=hair[2];break;
            case 's':r=skin[0];g=skin[1];b=skin[2];break;
            case 'e':r=238;g=231;b=211;break;
            case 'i':r=45;g=50;b=53;break;
            case 'c':r=coat[0];g=coat[1];b=coat[2];break;
            case 'd':r=coat[0]*3/5;g=coat[1]*3/5;b=coat[2]*3/5;break;
            case 'a':r=196;g=166;b=95;break;
            case 'm':r=200;g=191;b=156;break;
            case 'p':r=57;g=67;b=79;break;
            default:r=40;g=44;b=51;break;
            }
            int ox=px-8;
            if(ggOrigin.body==1&&py>=9&&py<=16)
                ox+=(px>=8?1:-1); // wider shoulders
            else if(ggOrigin.body==2&&py>=9&&py<=16)
                ox+=(px>=8?-1:1); // slimmer shoulders
            int wobble=(py>=17&&frame)?((px>=8)?1:-1):0;
            ggExpRect(out,x+(ox+wobble)*scale,y-(24-py)*scale,
                      scale,scale,r,g,b);
        }
    }
}
typedef struct { int x,feet,scale; } GgOriginMark;

static GgOriginMark ggOriginSet(u8 *out,int phase,int frame){
#if defined(__3DS__) && !defined(GG_FILM_2D_FALLBACK)
    int birth=phase==GG_ORIGIN_BIRTH,awakening=phase==GG_ORIGIN_AWAKEN;
    FilmBeat take={0};
    take.place=birth?F_PORCH:awakening?F_GATE:F_VILLAGE;
    take.camera=F_TRACK;
    take.lead=take.other=F_OFFSCREEN;
    take.prop=F_NO_PROP;
    int move=frame>120?256:frame*256/120;
    GgFilmCamera cam=ggFilmCameraAt(&take,move);
    // The globe and zoom buffer is idle while the first-person mode owns
    // the top LCD. Rendering the set there leaves the DS UI page intact.
    cam.page=gbuf;
    ggFilmWorldDraw(cam,&take,frame,0,1,0);
    int actorX=phase==GG_ORIGIN_YOUTH?48+move/2:96;
    GgV3 foot=ggV3(actorX,ggFilmSurfaceHeight(take.place,0,actorX,700),700);
    if(birth){
        ggFilmBox(cam,-116,0,690,10,57,99,24,20,25);
        ggFilmBox(cam,-12,0,690,10,57,99,24,20,25);
        ggFilmBox(cam,-64,49,690,121,17,101,25,24,26);
        ggFilmBox(cam,-64,63,689,102,7,71,23,21,22);
        ggFilmBox(cam,-116,59,689,8,37,92,25,24,26);
        ggFilmBox(cam,-12,59,689,8,29,92,25,24,26);
        foot=ggV3(-62,70,676);
    }
    if(awakening){
        int falling=frame<54?frame:54;
        int x=36+falling,z=770;
        int floor=ggFilmSurfaceHeight(take.place,0,x,z);
        foot=ggV3(x,floor+83-falling*83/54,z);
        ggFilmContactShadow(cam,ggV3(x,floor,z),take.place,0,24,37,falling==54);
        int caught=frame<38?frame:38;
        int beamY=219-caught*105/38;
        if(frame>84)beamY=114-(frame<122?frame-84:38)*108/38;
        ggFilmBox(cam,-16,beamY,820,140,14,18,25,24,26);
        // Wind passes between the traveler and the beam while it is held.
        // The ribbons narrow in the distance through the same projection.
        if(frame>=32&&frame<88){
            int flow=frame-32;
            for(int i=0;i<3;i++){
                int x=-138+(flow*5+i*49)%205;
                int y=76+i*16,z=748+i*30;
                ggFilmQuad(cam,ggV3(x,y,z),ggV3(x+56,y+10,z),
                               ggV3(x+56,y+15,z),ggV3(x,y+4,z),i&1?14:15);
            }
        }
    }else if(phase==GG_ORIGIN_CHILD||phase==GG_ORIGIN_YOUTH)
        ggFilmCastShadow(cam,foot,take.place,0);
    ggExpRect(out,0,0,400,240,17,24,36);
    u8 rgb[256][3]={0};
    for(unsigned ink=0;ink<GG_FILM_INK_N;ink++){
        unsigned c=ink<32?fmvPalette[ink]:ggFilmMaterialPalette[ink-32];
        unsigned slot=ggFilmInk((u8)ink);
        rgb[slot][0]=(u8)((c&31)*255/31);
        rgb[slot][1]=(u8)((c>>5&31)*255/31);
        rgb[slot][2]=(u8)((c>>10&31)*255/31);
    }
    for(int y=39;y<187;y++){
        const u8 *row=gbuf+(FILM_TOP+(y-39)*FILM_H/148)*256;
        u8 *dst=out+y*400*3;
        for(int x=0;x<400;x++,dst+=3){
            const u8 *c=rgb[row[x*256/400]];
            dst[0]=c[0];dst[1]=c[1];dst[2]=c[2];
        }
    }
    GgS2 point=ggFilmProject(cam,foot);
    return (GgOriginMark){point.x*400/256,39+(point.y-FILM_TOP)*148/FILM_H,
                         phase==GG_ORIGIN_YOUTH||awakening?2:1};
#else
    // The standalone exploration harness and diagnostic build can render
    // these shots with the existing first-person world camera.
    (void)phase;(void)frame;
    ggExpRenderMode(out,1);
    return (GgOriginMark){239,165,2};
#endif
}
static void ggOriginNewborn(u8 *out,int x,int feet){
    static const char *const swaddle[]={
        "...hhh........", "..hsssh.......", "..ssessttt....",
        "...ssstttttt..", "...ttttcttttt.", "....ttctttttt.",
        ".....ttttttt..",
    };
    const u8 *skin=ggOriginSkinRGB[ggOrigin.skin];
    const u8 *hair=ggOriginHairRGB[ggOrigin.hair];
    for(int row=0;row<7;row++)
        for(int col=0;col<14;col++){
            int ink=swaddle[row][col];
            if(ink=='.')continue;
            int px=x+(col-7)*2,py=feet+(row-7)*2;
            if(ink=='s')ggExpRect(out,px,py,2,2,skin[0],skin[1],skin[2]);
            else if(ink=='h'||ink=='e')ggExpRect(out,px,py,2,2,hair[0],hair[1],hair[2]);
            else if(ink=='c')ggExpRect(out,px,py,2,2,142,157,171);
            else ggExpRect(out,px,py,2,2,196,209,201);
        }
}
static void ggOriginStrip(u8 *out,const char *headline,const char *lineA,
                          const char *lineB,int frame){
    ggExpRect(out,0,0,400,39,17,24,36);
    ggExpRect(out,0,37,400,2,182,134,73);
    ggExpText(out,13,10,headline,242,215,159);
    ggExpRect(out,0,187,400,53,15,23,34);
    ggExpRect(out,0,187,400,2,174,128,73);
    ggExpText(out,12,198,lineA,224,226,221);
    ggExpText(out,12,212,lineB,180,201,212);
    if((frame/22)&1)ggExpText(out,317,231,"A CONTINUE",230,194,128);
}
static void ggOriginCreator(u8 *out){
    char value[80];
    ggOriginSet(out,GG_ORIGIN_CREATE,ggOrigin.frame);
    ggOriginPortrait(out,300,177,4,(ggOrigin.frame/16)&1,0);
    ggExpRect(out,0,0,218,240,18,30,42);
    ggExpRect(out,0,0,218,3,194,147,84);
    ggExpText(out,12,13,"YEAR 22 / YOUR ORDINARY LIFE",241,217,164);
    for(int i=0;i<6;i++){
        int y=45+i*25;
        if(i==ggOrigin.row)ggExpRect(out,7,y-4,205,22,58,75,83);
        ggExpText(out,13,y,ggOriginRows[i],211,211,196);
        if(i==0)snprintf(value,sizeof value,"%s",ggOriginNames[ggOrigin.name]);
        else if(i==1)snprintf(value,sizeof value,"%s",
                       ggOrigin.body==0?"AVERAGE":ggOrigin.body==1?"BROAD":"SLENDER");
        else if(i==2)snprintf(value,sizeof value,"%d / %d",ggOrigin.skin+1,GG_ORIGIN_SKINS);
        else if(i==3)snprintf(value,sizeof value,"%d / %d",ggOrigin.hair+1,GG_ORIGIN_HAIRS);
        else if(i==4)snprintf(value,sizeof value,"%d / %d",ggOrigin.garment+1,GG_ORIGIN_GARMENTS);
        else snprintf(value,sizeof value,"%s",ggOriginHomes[ggOrigin.home]);
        ggExpText(out,114,y,value,244,216,146);
    }
    ggExpText(out,11,198,"PAD ROW  LEFT/RIGHT CHOOSE",207,209,214);
    ggExpText(out,11,213,"A ACCEPT   NO CLASS LOCK-IN",243,208,140);
}
static void ggOriginDraw(u8 *out){
    if(!ggOrigin.active)return;
    int f=ggOrigin.frame,p=ggOrigin.phase;
    if(p==GG_ORIGIN_CREATE){ggOriginCreator(out);return;}
    if(p>=GG_ORIGIN_AWAKEN){
        if(p==GG_ORIGIN_AWAKEN){
            GgOriginMark cast=ggOriginSet(out,p,f);
            ggOriginPortrait(out,cast.x,cast.feet,cast.scale,f<54&&((f/8)&1),1);
        }else ggExpRenderMode(out,1);
        ggExpRect(out,0,0,400,40,16,25,41);
        ggExpRect(out,0,39,400,2,193,147,90);
        ggExpText(out,12,11,"YEAR 22 / THE HIDDEN HALF",242,220,162);
        ggExpRect(out,0,175,400,65,13,23,36);
        ggExpRect(out,0,175,400,2,185,130,70);
        if(p==GG_ORIGIN_AWAKEN){
            ggExpText(out,13,190,"A STRANGER FALLS. YOU REACH INTO THE WIND.",238,216,171);
            ggExpText(out,13,207,"THE WORLD ANSWERS. YOU DID NOT KNOW IT COULD.",219,231,234);
        }else{
            ggExpText(out,13,188,"NO PROPHECY RAISED YOU. NO THRONE AWAITS YOU.",244,213,167);
            ggExpText(out,13,203,"YOUR GIFT IS REAL. YOUR PLACE IS STILL YOURS TO MAKE.",216,222,223);
            ggExpText(out,13,220,"WORK, WANDER, LEAD, OR DISAPPEAR. A CONTINUE.",181,212,221);
        }
        return;
    }
    GgOriginMark cast=ggOriginSet(out,p,f);
    if(p==GG_ORIGIN_BIRTH){
        ggOriginNewborn(out,cast.x,cast.feet);
        ggOriginStrip(out,"YEAR 0 / NO SIGN IN THE SKY",
            "A CHILD WAS BORN UNDER AN ORDINARY ROOF.",
            "NEITHER PARENT SPOKE OF GODS. THERE WAS NO OMEN.",f);
    }else if(p==GG_ORIGIN_CHILD){
        ggOriginPortrait(out,cast.x,cast.feet,cast.scale,(f/12)&1,0);
        ggOriginStrip(out,"YEAR 8 / SMALL FOOTSTEPS",
            "YOU LEARNED THE FIELDS, THE WEATHER, AND THE ROAD.",
            "WHEN OTHERS CALLED YOU ORDINARY, YOU BELIEVED THEM.",f);
    }else{
        ggOriginPortrait(out,cast.x,cast.feet,cast.scale,(f/10)&1,0);
        ggOriginStrip(out,"YEAR 16 / AN UNREMARKABLE FUTURE",
            "WORK, FRIENDS, AND SMALL AMBITIONS FILLED THE YEARS.",
            "NO ONE KNEW WHAT SLEPT IN YOUR BLOOD. NOT EVEN YOU.",f);
    }
}
#endif
