// GodGuy: The Unwritten Reign - 3DS original opening and character creator.
// The playable character is born ordinary to everyone who knows them.
// Their dormant demigod nature is revealed at age 22, not at birth.
// All artwork is lightweight procedural pixel scenery / original cel shapes.
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
// Fixed-work animated art. Everything is painted onto the live top RGB
// surface, so the original DS story art and 3DS video RAM are untouched.
static void ggOriginSky(u8 *out,int dusk,int f){
    for(int y=0;y<240;y++){
        int r=dusk?25+y/7:50+y/3;
        int g=dusk?39+y/5:75+y/3;
        int b=dusk?69+y/6:97+y/3;
        ggExpRect(out,0,y,400,1,r,g,b);
    }
    ggExpRect(out,0,171,400,69,dusk?24:46,dusk?47:79,dusk?45:52);
    for(int i=0;i<6;i++){
        int x=i*88-35+(f/8)%8;
        ggExpRect(out,x,162+(i*7)%19,110,10,
                  dusk?37:68,dusk?55:99,dusk?64:76);
    }
}
static void ggOriginTree(u8 *out,int x,int y,int f){
    ggExpRect(out,x-3,y-29,6,30,80,66,56);
    ggExpRect(out,x-15,y-50,30,24,36,82,59);
    ggExpRect(out,x-24,y-43,18,18,43,97,58);
    ggExpRect(out,x+8,y-38,20,16,51,101,63);
    if((f/16)&1)ggExpRect(out,x+13,y-40,4,5,101,139,77);
}
static void ggOriginHouse(u8 *out,int x,int y,int scale){
    int w=65*scale/2,h=46*scale/2;
    ggExpRect(out,x,y-h,w,h,152,128,96);
    ggExpRect(out,x-6,y-h-10,w+12,10,91,61,61);
    ggExpRect(out,x+w/2-9,y-25*scale/2,18,25*scale/2,69,57,61);
    ggExpRect(out,x+8,y-h+10,12,13,207,184,124);
    ggExpRect(out,x+10,y-h+12,8,9,66,90,109);
    ggExpRect(out,x+14,y-h+12,2,9,142,115,88);
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
static void ggOriginPortrait(u8 *out,int x,int y,int scale,int frame,int small){
    (void)small;
    const u8 *skin=ggOriginSkinRGB[ggOrigin.skin];
    const u8 *hair=ggOriginHairRGB[ggOrigin.hair];
    const u8 *coat=ggOriginClothesRGB[ggOrigin.garment];
    ggExpRect(out,x-8*scale,y-2,16*scale,3,30,38,40);
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
    ggOriginSky(out,0,ggOrigin.frame);
    ggOriginTree(out,341,183,ggOrigin.frame);
    ggOriginHouse(out,254,177,2);
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
        ggExpRender(out); // The real world, not an imaginary new map.
        ggExpRect(out,0,0,400,40,16,25,41);
        ggExpRect(out,0,39,400,2,193,147,90);
        ggExpText(out,12,11,"YEAR 22 / THE HIDDEN HALF",242,220,162);
        int bolt=(f*7)%400;
        // The first miracle is a gust that stops falling debris.
        // It does not imply the child had visible powers at birth.
        if(p==GG_ORIGIN_AWAKEN){
            for(int i=0;i<6;i++){
                int x=(bolt+i*71)%400,y=77+(i*19)%88;
                ggExpRect(out,x,y,20+(i%3)*8,2,202,202,180);
            }
            if((f/6)%4==0)ggExpRect(out,192,77,13,48,237,210,150);
        }
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
    ggOriginSky(out,p==GG_ORIGIN_BIRTH,f);
    if(p==GG_ORIGIN_BIRTH){
        ggExpRect(out,36,65,330,124,127,101,84);
        ggExpRect(out,39,69,325,8,92,76,73);
        ggExpRect(out,70,133,240,10,110,76,67);
        ggExpRect(out,86,146,110,30,79,70,74);
        ggExpRect(out,102,140,70,11,194,185,168);
        ggExpRect(out,117,133,29,17,215,160,122);
        ggExpRect(out,115,131,31,6,77,56,53);
        ggExpRect(out,278,96,14,82,67,72,78);
        ggExpRect(out,257,87,52,12,119,104,86);
        ggOriginStrip(out,"YEAR 0 / NO SIGN IN THE SKY",
            "A CHILD WAS BORN UNDER AN ORDINARY ROOF.",
            "NEITHER PARENT SPOKE OF GODS. THERE WAS NO OMEN.",f);
    }else if(p==GG_ORIGIN_CHILD){
        ggOriginHouse(out,49,169,2);
        ggOriginTree(out,318,176,f);
        ggExpRect(out,205,157,65,7,119,111,78);
        ggOriginPortrait(out,242,176,2,(f/12)&1,1);
        ggOriginStrip(out,"YEAR 8 / SMALL FOOTSTEPS",
            "YOU LEARNED THE FIELDS, THE WEATHER, AND THE ROAD.",
            "WHEN OTHERS CALLED YOU ORDINARY, YOU BELIEVED THEM.",f);
    }else{
        // Years later, this is still the SAME house, tree and road
        // from the childhood shot. Age progression must not magically
        // reverse geography or throw the player into a new background.
        ggOriginHouse(out,49,169,2);
        ggOriginTree(out,318,176,f);
        for(int i=0;i<8;i++){
            int x=110+i*13;
            ggExpRect(out,x,155+(i&1)*2,2,19,101,130,63);
            ggExpRect(out,x-2,150+(i&1)*2,6,8,173,157,71);
        }
        int walk=ggExpClamp(f/5,0,16);
        ggOriginPortrait(out,220+walk,180,3,
                         walk<16?(f/10)&1:0,0);
        ggOriginStrip(out,"YEAR 16 / AN UNREMARKABLE FUTURE",
            "WORK, FRIENDS, AND SMALL AMBITIONS FILLED THE YEARS.",
            "NO ONE KNEW WHAT SLEPT IN YOUR BLOOD. NOT EVEN YOU.",f);
    }
}
#endif
