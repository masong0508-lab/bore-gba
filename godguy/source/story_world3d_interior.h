#ifndef GG_STORY_WORLD3D_INTERIOR_H
#define GG_STORY_WORLD3D_INTERIOR_H

enum {
    GG_FILM_PORCH_TABLE_Z=583, GG_FILM_PORCH_TABLE_TOP=36, GG_FILM_PORCH_TABLE_D=74,
    GG_FILM_ARCHIVE_DESK_TOP=55, GG_FILM_ARCHIVE_DESK_Z=840, GG_FILM_ARCHIVE_DESK_D=106,
    GG_FILM_ANVIL_TOP=93, GG_FILM_ANVIL_Z=820, GG_FILM_ANVIL_D=57,
    GG_FILM_COUCH_X=175, GG_FILM_COUCH_Z=920, GG_FILM_COUCH_CUSHION_TOP=32,
    GG_FILM_COUCH_TABLE_Z=678, GG_FILM_COUCH_TABLE_TOP=34, GG_FILM_COUCH_TABLE_D=69,
    GG_FILM_THRONE_STAND_Z=697, GG_FILM_THRONE_STAND_TOP=43, GG_FILM_THRONE_STAND_D=69
};

static void ggFilmLowTable(GgFilmCamera cam,int x,int z,int w,int d,int top){
    for(int end=1;end>=-1;end-=2){
        for(int side=-1;side<=1;side+=2){
            int px=x+side*(w/2-15),pz=z+end*(d/2-10);
            ggFilmBox(cam,px,1,pz,10,top-7,12,25,24,26);
            ggFilmBox(cam,px,1,pz,13,5,14,24,GG_OXBLOOD,25);
        }
    }
    ggFilmBox(cam,x,top-15,z,w-12,9,d-11,25,24,26);
    ggFilmBox(cam,x,top-6,z,w,6,d,26,24,GG_PLASTER);
    for(int i=1;i<=3;i++){
        int grain=z-d/2+i*d/5;
        ggFilmGroundPatch(cam,x-(i%2)*15,top,grain,w-25,4,26);
    }
}
static void ggFilmTableOnFloor(GgFilmCamera cam,int x,int z,int w,int d,int top){
    ggFilmGroundPatch(cam,x+13,1,z,w+34,d+28,GG_OXBLOOD);
    ggFilmLowTable(cam,x,z,w,d,top);
}
static int ggFilmWoodRoom(int p){return p==F_PORCH||p==F_COUCH||p==F_HEARTH;}
static void ggFilmRoomFloor(GgFilmCamera cam,int p){
    int wood=ggFilmWoodRoom(p);
    u8 base=wood?24:p==F_HALL?GG_STONE:GG_SLATE;
    ggFilmPaving(cam,-850,850,327,1700,0,base);
    if(wood){
        for(int i=0;i<17;i++){
            int x=-770+i*93,z=497+i*214%490;
            u8 top=p==F_PORCH?(i%4?25:26):p==F_HEARTH?(i%3?25:GG_OXBLOOD):(i%3?25:24);
            ggFilmPaving(cam,x+3,x+89,327,z,1,top);
            ggFilmPaving(cam,x+3,x+89,z+5,1700,1,i%3?25:GG_OXBLOOD);
            ggFilmPaving(cam,x+11,x+16,z-147,z-39,2,24);
            ggFilmPaving(cam,x+68,x+74,z+44,z+113,2,26);
        }
        return;
    }
    for(int row=0,z=355;z<1700;row++,z+=153){
        int cap=z+150;
        if(cap>1700)cap=1700;
        for(int x=-875-(row&1)*82,i=0;x<850;x+=171,i++){
            int style=(row*3+i*7)%11;
            u8 fill=style<3?GG_STONE:style<6?21:p==F_HALL?GG_LIMESTONE:GG_SLATE;
            ggFilmPaving(cam,x+3,x+168,z,cap,1,fill);
            if(style<7)ggFilmPaving(cam,x+6,x+159,z+7,z+13,2,
                                    p==F_HALL?GG_STONE:GG_SLATE);
        }
    }
}
static void ggFilmRug(GgFilmCamera cam,int x,int n,int f,int w,
                      u8 edge,u8 border,u8 field){
    int l=x-w/2,r=x+w/2;
    ggFilmPaving(cam,l-7,r+7,n-8,f+7,2,24);
    ggFilmPaving(cam,l,r,n,f,3,edge);
    ggFilmPaving(cam,l+12,r-12,n+24,f-24,4,border);
    ggFilmPaving(cam,l+22,r-22,n+37,f-37,5,field);
    for(int i=0;i<9;i++){
        int xx=l+10+(w-20)*i/8;
        ggFilmPaving(cam,xx-4,xx+4,n-12,n+2,3,edge);
    }
    for(int z=n+91;z<f-37;z+=152)
        ggFilmQuad(cam,ggV3(x-30,6,z),ggV3(x,6,z-45),
                      ggV3(x+30,6,z),ggV3(x,6,z+45),border);
}
static void ggFilmRoomShell(GgFilmCamera cam,int p){
    int warm=ggFilmWoodRoom(p),back=1600;
    u8 wall=p==F_COUCH?GG_OXBLOOD:p==F_PORCH?26:p==F_HEARTH?GG_PLASTER:
            p==F_THRONE?GG_SKY_RAIN:p==F_HALL?GG_STONE:
            p==F_FORGE?GG_SLATE:GG_PINE;
    ggFilmPanel(cam,-704,704,0,470,back,wall);
    ggFilmQuad(cam,ggV3(-704,0,340),ggV3(-704,0,back),
                  ggV3(-704,470,back),ggV3(-704,470,340),warm?24:20);
    ggFilmQuad(cam,ggV3(704,0,back),ggV3(704,0,340),
                  ggV3(704,470,340),ggV3(704,470,back),warm?25:GG_SLATE);
    if(!warm){
        ggFilmMasonry(cam,-699,699,18,468,back-1,73,126,
                      p==F_HALL?21:20,wall,p==F_HALL?GG_STONE:wall);
    }else{
        for(int y=24;y<447;y+=52){
            ggFilmPanel(cam,-699,699,y,y+4,back-1,24);
            for(int i=0;i<6;i++){
                int x=-630+(i*253+y*7)%1310;
                ggFilmGrain(cam,x,x+74+(i%3)*38,y+13+i%2*12,back-2,4,
                            p==F_HEARTH?26:p==F_COUCH?24:25);
            }
        }
    }
    ggFilmPanel(cam,-700,700,0,37,back-3,warm?24:GG_SLATE);
    ggFilmPanel(cam,-700,700,37,47,back-4,warm?26:GG_STONE);
    ggFilmQuad(cam,ggV3(-704,448,back),ggV3(704,448,back),
                  ggV3(704,448,400),ggV3(-704,448,400),warm?24:20);
    ggFilmRoomFloor(cam,p);
}
// Doorheads, niches and furnace mouths all have a projecting carved rim.
static void ggFilmRoundTop(GgFilmCamera cam,int x,int z,int y,int w,int h,u8 ink){
    int l=x-w/2,r=x+w/2,shoulder=y+h*2/3;
    ggFilmPanel(cam,l,r,y,shoulder,z,ink);
    GgV3 m=ggV3(x,shoulder,z);
    GgV3 fan[]={ggV3(l,shoulder,z),ggV3(x-w*2/5,y+h*7/8,z),
        ggV3(x-w/5,y+h*31/32,z),ggV3(x,y+h,z),
        ggV3(x+w/5,y+h*31/32,z),ggV3(x+w*2/5,y+h*7/8,z),
        ggV3(r,shoulder,z)};
    for(int i=0;i<6;i++)ggFilmFace(cam,m,fan[i],fan[i+1],ink);
}
static void ggFilmWindow(GgFilmCamera cam,int x,int z,int y,int w,int h){
    ggFilmRoundTop(cam,x,z,y-5,w+26,h+18,GG_STONE);
    ggFilmRoundTop(cam,x,z-2,y,w+12,h+9,GG_SLATE);
    ggFilmRoundTop(cam,x,z-3,y+5,w,h,GG_SKY_MIST);
    ggFilmPanel(cam,x-w/2+3,x+w/2-3,y+7,y+h/4,z-5,GG_SKY_HIGH);
    for(int i=-1;i<=1;i+=2)
        ggFilmPanel(cam,x+i*w/4-2,x+i*w/4+2,y+5,y+h*9/10,z-6,GG_SLATE);
    ggFilmPanel(cam,x-w/2,x+w/2,y+h/3,y+h/3+5,z-6,GG_SLATE);
    ggFilmPanel(cam,x-w/2,x+w/2,y+h*2/3,y+h*2/3+5,z-6,GG_SLATE);
    ggFilmBox(cam,x,y-6,z-14,w+37,10,31,GG_STONE,21,GG_LIMESTONE);
}
static void ggFilmBanner(GgFilmCamera cam,int x,int z,int y,int h,int w,u8 cloth){
    int l=x-w/2,r=x+w/2;
    ggFilmBox(cam,x,y+h,z,w+39,9,13,25,24,26);
    ggFilmPanel(cam,l,r,y+21,y+h,z-8,cloth);
    ggFilmFace(cam,ggV3(l,y+21,z-8),ggV3(r,y+21,z-8),ggV3(x,y,z-8),cloth);
    ggFilmPanel(cam,l+7,l+13,y+23,y+h-10,z-9,26);
    ggFilmPanel(cam,r-13,r-7,y+23,y+h-10,z-9,26);
    ggFilmPanel(cam,x-6,x+5,y+55,y+h-35,z-10,GG_PLASTER);
    ggFilmQuad(cam,ggV3(x-27,y+h/2,z-11),ggV3(x,y+h/2+26,z-11),
                  ggV3(x+27,y+h/2,z-11),ggV3(x,y+h/2-27,z-11),26);
    ggFilmFace(cam,ggV3(x-19,y+h/2,z-12),ggV3(x,y+h/2+19,z-12),
                  ggV3(x+19,y+h/2,z-12),9);
}
static void ggFilmFire(GgFilmCamera cam,int x,int y,int z,int w,int h,int t){
    ggFilmPanel(cam,x-w/2,x+w/2,y-6,y+10,z,GG_OXBLOOD);
    for(int i=0;i<5;i++){
        int q=x+(i-2)*w/6,rise=h*(7+(t/6+i*7)%8)/15;
        ggFilmFace(cam,ggV3(q-w/7,y,z),ggV3(q+w/6,y,z),
                      ggV3(q+(i%2?8:-6),y+rise,z-2),i&1?7:8);
        ggFilmFace(cam,ggV3(q-w/10,y,z-3),ggV3(q+w/9,y,z-3),
                      ggV3(q+4,y+rise/2,z-3),i&1?9:10);
    }
}
static void ggFilmLamp(GgFilmCamera cam,int x,int y,int z,int t){
    ggFilmBox(cam,x,y-9,z,28,8,21,24,GG_OXBLOOD,26);
    ggFilmBox(cam,x,y-17,z,7,22,9,25,24,26);
    ggFilmFire(cam,x,y,z-9,19,28,t);
}
static void ggFilmHearth(GgFilmCamera cam,int x,int z,int w,int h,int t,int forge){
    int f=z-53, jamb=forge?76:57;
    ggFilmBox(cam,x,0,z,w,h,106,forge?GG_SLATE:GG_STONE,20,GG_LIMESTONE);
    ggFilmMasonry(cam,x-w/2,x+w/2,0,h,f-1,35,51,20,GG_STONE,
                  forge?21:GG_LIMESTONE);
    ggFilmRoundTop(cam,x,f-2,19,w-jamb,h*3/4,forge?GG_COPPER:GG_PLASTER);
    ggFilmRoundTop(cam,x,f-4,20,w-jamb-24,h*3/4-14,2);
    int half=(w-jamb-24)/2,back=f+41,sill=21;
    GgV3 nearL=ggV3(x-half,sill,f-5),nearR=ggV3(x+half,sill,f-5);
    GgV3 farL=ggV3(x-half+21,sill,back),farR=ggV3(x+half-21,sill,back);
    ggFilmQuad(cam,nearL,nearR,farR,farL,GG_OXBLOOD);
    ggFilmQuad(cam,nearL,farL,ggV3(farL.x,86,back),ggV3(nearL.x,81,f-5),24);
    ggFilmQuad(cam,farR,nearR,ggV3(nearR.x,81,f-5),ggV3(farR.x,86,back),GG_SLATE);
    ggFilmBox(cam,x-13,sill,back-11,56,10,21,7,GG_OXBLOOD,8);
    ggFilmBox(cam,x+19,sill,back-21,43,12,14,GG_COPPER,24,8);
    ggFilmBox(cam,x,10,z-72,w+34,11,88,GG_SLATE,20,GG_STONE);
    ggFilmBox(cam,x,h-8,z-13,w+31,15,122,25,20,26);
    ggFilmFire(cam,x,29,back-23,w-jamb-51,h/3,t);
}
static void ggFilmPorch(GgFilmCamera cam,int t,int chapter,int winter){
    int rain=chapter==1||winter;
    // Window light follows a fixed projected footprint on the floor.
    ggFilmQuad(cam,ggV3(68,2,980),ggV3(311,2,980),
                  ggV3(461,2,1480),ggV3(218,2,1480),rain?GG_SKY_RAIN:GG_PLASTER);
    ggFilmQuad(cam,ggV3(216,3,966),ggV3(232,3,966),
                  ggV3(357,3,1480),ggV3(337,3,1480),25);
    ggFilmBox(cam,0,282,1490,1300,22,75,24,20,26);
    for(int x=-560;x<=560;x+=280){
        ggFilmBox(cam,x,0,1180,30,272,39,24,20,26);
        ggFilmBox(cam,x,252,1178,46,29,47,25,24,26);
        ggFilmBox(cam,x,309,1130,15,24,710,24,25,26);
    }
    ggFilmBox(cam,0,267,1179,1170,17,46,25,24,26);
    // The hinged door and its timber panels open together in the first rain.
    int open=chapter==1?(t<300?t/12:25):0;
    ggFilmPanel(cam,-555,-425,0,213,1545,2);
    ggFilmPanel(cam,-550,-496,19,195,1544,rain?GG_SKY_RAIN:GG_SKY_MIST);
    ggFilmQuad(cam,ggV3(-551,2,1532),ggV3(-432-open,2,1532-open*3),
                  ggV3(-432-open,201,1532-open*3),ggV3(-551,201,1532),25);
    for(int i=1;i<=4;i++){
        int x=-551+i*(119-open)/5,z=1531-i*open*3/5;
        ggFilmPanel(cam,x,x+3,5,198,z,24);
    }
    ggFilmBox(cam,-478-open,104,1528-open*3,8,6,8,9,24,26);
    ggFilmBox(cam,-555,0,1539,12,213,18,24,25,26);
    ggFilmBox(cam,-428,0,1539,12,213,18,24,25,26);
    ggFilmBox(cam,-492,210,1539,147,14,20,24,25,26);
    ggFilmPanel(cam,166,503,108,260,1557,24);
    ggFilmPanel(cam,178,492,116,253,1554,rain?GG_SKY_RAIN:GG_SKY_HIGH);
    ggFilmFace(cam,ggV3(180,116,1552),ggV3(490,116,1552),
                  ggV3(259,190,1552),rain?GG_PINE:GG_LEAF);
    ggFilmFace(cam,ggV3(319,116,1551),ggV3(491,116,1551),
                  ggV3(491,169,1551),rain?31:GG_PINE);
    if(rain)
        for(int i=0;i<13;i++){
            int x=187+i*25,y=125+((i*47+t/5)%105);
            ggFilmPanel(cam,x,x+(winter?7:3),y,y+(winter?7:15),1550,
                        winter?23:GG_WATER_CREST);
        }
    ggFilmBox(cam,331,108,1535,333,12,34,24,25,26);
    ggFilmPanel(cam,168,501,250,262,1532,25);
    for(int x=180;x<=492;x+=78)ggFilmPanel(cam,x-4,x+4,112,252,1531,25);
    ggFilmPanel(cam,174,498,182,189,1530,25);
    int flutter=((t/36)%7)-3;
    for(int side=-1;side<=1;side+=2){
        int x=332+side*162,hem=x+side*flutter;
        ggFilmQuad(cam,ggV3(x-22,252,1518),ggV3(x+22,252,1518),
                      ggV3(hem+17,121,1518),ggV3(hem-21,121,1518),GG_PINE);
        ggFilmQuad(cam,ggV3(x-14,251,1517),ggV3(x+1,251,1517),
                      ggV3(hem+5,122,1517),ggV3(hem-7,122,1517),GG_LEAF);
    }
    ggFilmBox(cam,-260,0,1116,206,31,71,24,24,26);
    ggFilmBox(cam,-260,28,1135,206,65,19,25,24,26);
    ggFilmGrain(cam,-338,-181,55,1124,4,26);
    ggFilmRug(cam,-151,465,856,137,26,GG_COPPER,GG_OXBLOOD);
    ggFilmTableOnFloor(cam,0,GG_FILM_PORCH_TABLE_Z,145,GG_FILM_PORCH_TABLE_D,GG_FILM_PORCH_TABLE_TOP);
    ggFilmLamp(cam,456,137,1525,t);
}

static void ggFilmBooks(GgFilmCamera cam,int x,int z,int w,int top){
    int start=x-w/2,end=x+w/2,front=z-18;
    ggFilmBox(cam,x,0,z,w,top,63,24,GG_OXBLOOD,26);
    ggFilmPanel(cam,start+11,end-11,18,top-10,front-1,2);
    for(int y=46,row=0;y<top-12;y+=53,row++){
        ggFilmBox(cam,x,y,front-11,w+7,7,25,25,24,26);
        for(int i=0;i<9;i++){
            int bx=start+14+i*(w-21)/9;
            int h=17+(i*13+row*7)%25;
            if((i+row*3)%11==7)continue;
            static const u8 covers[]={GG_COPPER,GG_PINE,30,25,GG_PLASTER,
                                      GG_OXBLOOD,31,GG_LEAF,21};
            u8 color=covers[(i*7+row*3)%9];
            ggFilmPanel(cam,bx,bx+10+(i%4),y+7,y+7+h,front-14,color);
            ggFilmPanel(cam,bx+1,bx+9,y+h-1,y+h+2,front-15,i%3?26:9);
        }
    }
    ggFilmBox(cam,x,top,z-5,w+18,11,81,25,24,26);
}
static void ggFilmArchiveDesk(GgFilmCamera cam,int t){
    ggFilmBox(cam,-55,0,835,19,52,84,25,24,26);
    ggFilmBox(cam,80,0,835,19,52,84,25,24,26);
    ggFilmBox(cam,12,42,GG_FILM_ARCHIVE_DESK_Z,171,GG_FILM_ARCHIVE_DESK_TOP-42,
              GG_FILM_ARCHIVE_DESK_D,25,24,26);
    ggFilmBox(cam,12,28,819,161,21,12,24,20,26);
    int page=((t/44)%11)-5;
    ggFilmQuad(cam,ggV3(-52,57,784),ggV3(7,57,784),
                  ggV3(7,61,862),ggV3(-53,62,861),GG_PLASTER);
    ggFilmQuad(cam,ggV3(7,57,784),ggV3(53,59,787),
                  ggV3(53,64+page,864),ggV3(7,61,862),23);
    for(int z=799;z<857;z+=12){
        ggFilmPaving(cam,-45,-4,z,z+3,61,26);
        ggFilmPaving(cam,14,46,z,z+3,62+page/3,GG_STONE);
    }
    ggFilmLamp(cam,-65,GG_FILM_ARCHIVE_DESK_TOP+17,860,t);
}
static void ggFilmArchive(GgFilmCamera cam,int t){
    ggFilmQuad(cam,ggV3(-373,2,564),ggV3(-249,2,564),
                  ggV3(-122,2,1480),ggV3(-247,2,1480),GG_STONE);
    ggFilmWindow(cam,-87,1560,141,117,226);
    ggFilmPanel(cam,30,617,254,269,1481,25);
    ggFilmBooks(cam,187,1446,212,271);
    ggFilmBooks(cam,432,1425,218,288);
    ggFilmBooks(cam,-410,1350,222,257);
    ggFilmRug(cam,36,556,1201,314,26,GG_COPPER,GG_PINE);
    // Desk and book share the projected writing prop's support.
    ggFilmGroundPatch(cam,47,6,816,236,141,24);
    ggFilmArchiveDesk(cam,t);
}
static void ggFilmForgeAnvil(GgFilmCamera cam){
    ggFilmBox(cam,-155,0,820,55,75,75,25,24,26);
    ggFilmBox(cam,-155,65,820,92,14,54,GG_SLATE,20,GG_STONE);
    ggFilmBox(cam,-155,73,GG_FILM_ANVIL_Z,133,GG_FILM_ANVIL_TOP-73,
              GG_FILM_ANVIL_D,GG_STONE,GG_SLATE,GG_LIMESTONE);
    ggFilmFace(cam,ggV3(-252,83,792),ggV3(-220,73,792),
                  ggV3(-219,93,792),GG_STONE);
    ggFilmFace(cam,ggV3(-252,83,791),ggV3(-220,84,791),
                  ggV3(-219,93,791),GG_LIMESTONE);
}
static void ggFilmForge(GgFilmCamera cam,int t){
    // Heat catches the stone facing the vent and leaves the anvil in silhouette.
    ggFilmQuad(cam,ggV3(-84,3,770),ggV3(428,3,770),
                  ggV3(474,3,1330),ggV3(80,3,1330),GG_OXBLOOD);
    ggFilmQuad(cam,ggV3(92,4,897),ggV3(399,4,897),
                  ggV3(373,4,1310),ggV3(144,4,1310),25);
    for(int i=0;i<5;i++){
        int z=890+i*75;
        ggFilmPaving(cam,102+i*14,412,z,z+4,5,GG_OXBLOOD);
    }
    ggFilmBox(cam,238,158,1417,110,229,113,GG_SLATE,20,GG_STONE);
    ggFilmMasonry(cam,183,293,165,380,1359,46,46,20,21,GG_SLATE);
    ggFilmHearth(cam,238,1355,229,175,t,1);
    ggFilmBox(cam,-299,0,1320,228,57,100,25,24,26);
    ggFilmPanel(cam,-403,-191,58,161,1378,24);
    for(int i=0;i<5;i++){
        int x=-383+i*41;
        ggFilmPanel(cam,x-3,x+4,90+i%3*5,145,1376,GG_STONE);
        ggFilmPanel(cam,x-11,x+11,142,153+i%3*5,1375,GG_SLATE);
    }
    ggFilmRock(cam,-184,0,1175,109,45,89,GG_STONE,GG_SLATE,20);
    ggFilmRock(cam,-116,0,1130,86,28,52,GG_COPPER,21,20);
    ggFilmGroundPatch(cam,-137,5,807,161,113,20);
    ggFilmForgeAnvil(cam);
    int compression=(t/23)%9;
    ggFilmBox(cam,387,0,1329,71,46,49,25,24,26);
    ggFilmBox(cam,380,45,1326,79,22-compression,45,GG_OXBLOOD,24,26);
    ggFilmBeam(cam,378,61-compression,1288,9,8,67,26);
}
static void ggFilmCourt(GgFilmCamera cam,int p,int t){
    int throne=p==F_THRONE;
    ggFilmRug(cam,0,397,1550,throne?275:397,26,GG_COPPER,
              throne?GG_OXBLOOD:GG_PINE);
    for(int side=-1;side<=1;side+=2){
        int x=side*392;
        ggFilmWindow(cam,x,1550,137,136,251);
        ggFilmQuad(cam,ggV3(x-104,2,808),ggV3(x+91,2,808),
                      ggV3(x+64,2,1450),ggV3(x-64,2,1450),throne?21:GG_LIMESTONE);
        ggFilmBox(cam,side*514,0,1270,93,378,92,GG_STONE,GG_SLATE,GG_LIMESTONE);
        ggFilmPanel(cam,side*514-35,side*514-20,30,370,1221,GG_LIMESTONE);
        ggFilmBox(cam,side*514,25,1270,122,14,113,GG_LIMESTONE,21,GG_STONE);
        ggFilmBox(cam,side*514,348,1270,117,19,115,GG_LIMESTONE,21,22);
        ggFilmBanner(cam,side*213,1440,127,206,108,throne?GG_OXBLOOD:GG_PINE);
        ggFilmBox(cam,side*191,0,1218,22,116,22,25,24,26);
        ggFilmLamp(cam,side*191,125,1214,t+side*15);
    }
    ggFilmBox(cam,0,0,1498,347,17,252,GG_STONE,GG_SLATE,GG_LIMESTONE);
    ggFilmBox(cam,0,17,1517,288,17,205,GG_STONE,GG_SLATE,GG_LIMESTONE);
    ggFilmBox(cam,0,34,1529,238,14,164,GG_STONE,GG_SLATE,GG_LIMESTONE);
    if(throne){
        ggFilmRoundTop(cam,0,1511,126,226,257,GG_SLATE);
        ggFilmRoundTop(cam,0,1509,136,185,221,GG_OXBLOOD);
        ggFilmBox(cam,0,48,1460,175,62,91,25,24,26);
        ggFilmRoundTop(cam,0,1456,83,121,197,26);
        ggFilmRoundTop(cam,0,1455,92,96,169,GG_COPPER);
        ggFilmPanel(cam,-37,37,108,235,1453,GG_OXBLOOD);
        for(int side=-1;side<=1;side+=2){
            ggFilmBox(cam,side*84,56,1430,30,77,81,25,24,26);
            ggFilmBox(cam,side*67,236,1454,14,42,16,26,25,9);
        }
        ggFilmBox(cam,0,267,1452,24,33,21,26,GG_COPPER,9);
        ggFilmTableOnFloor(cam,0,GG_FILM_THRONE_STAND_Z,118,GG_FILM_THRONE_STAND_D,GG_FILM_THRONE_STAND_TOP);
    }else{
        ggFilmBox(cam,0,50,1470,226,64,65,25,24,26);
        ggFilmPanel(cam,-96,96,58,104,1436,24);
        for(int x=-65;x<=65;x+=65)ggFilmPanel(cam,x-11,x+11,64,103,1435,26);
        ggFilmBox(cam,0,111,1468,244,9,85,25,24,GG_PLASTER);
        ggFilmBox(cam,-145,46,1523,67,132,54,GG_STONE,GG_SLATE,GG_LIMESTONE);
        ggFilmBox(cam,145,46,1523,67,132,54,GG_STONE,GG_SLATE,GG_LIMESTONE);
    }
}
static void ggFilmSofa(GgFilmCamera cam,int x,int z,int hearth){
    u8 cloth=hearth?GG_COPPER:GG_PINE, crease=hearth?GG_OXBLOOD:16;
    int seat=GG_FILM_COUCH_CUSHION_TOP;
    ggFilmGroundPatch(cam,x+26,1,z,282,137,24);
    for(int side=-1;side<=1;side+=2)
        for(int end=-1;end<=1;end+=2)
            ggFilmBox(cam,x+side*86,1,z+end*32,15,13,17,24,GG_OXBLOOD,25);
    ggFilmBox(cam,x,12,z,220,seat-17,83,crease,24,cloth);
    ggFilmBox(cam,x,seat-6,z+37,220,98-seat,25,cloth,24,hearth?26:GG_LEAF);
    for(int sx=-1;sx<=1;sx++){
        int spot=x+sx*70;
        ggFilmBox(cam,spot,seat-7,z-9,67,7,67,cloth,crease,hearth?26:GG_LEAF);
        ggFilmPanel(cam,spot-27,spot+27,46,89,z+21,crease);
        ggFilmPanel(cam,spot-23,spot+23,49,86,z+20,cloth);
        ggFilmPanel(cam,spot-3,spot+3,67,72,z+19,hearth?26:GG_MOSS);
        ggFilmGrain(cam,spot-21,spot+23,52,z+19,3,hearth?25:GG_LEAF);
    }
    for(int side=-1;side<=1;side+=2){
        ggFilmBox(cam,x+side*112,12,z,25,49,106,crease,24,cloth);
        ggFilmBox(cam,x+side*112,55,z-5,31,12,104,cloth,crease,hearth?26:GG_LEAF);
    }
}
static void ggFilmSittingRoom(GgFilmCamera cam,int p,int t){
    int hearth=p==F_HEARTH;
    ggFilmRug(cam,hearth?-115:120,469,1170,445,26,GG_COPPER,
              hearth?GG_OXBLOOD:GG_PINE);
    if(hearth){
        ggFilmQuad(cam,ggV3(-504,7,838),ggV3(24,7,838),
                      ggV3(-71,7,1440),ggV3(-420,7,1440),GG_COPPER);
        ggFilmQuad(cam,ggV3(-445,8,1061),ggV3(-72,8,1061),
                      ggV3(-97,8,1422),ggV3(-392,8,1422),26);
        ggFilmBox(cam,-260,163,1515,165,224,82,GG_STONE,GG_SLATE,GG_LIMESTONE);
        ggFilmMasonry(cam,-342,-178,167,389,1472,40,51,21,GG_LIMESTONE,GG_STONE);
        ggFilmHearth(cam,-260,1470,251,173,t,0);
        ggFilmRock(cam,-405,0,1250,109,46,72,26,25,24);
        ggFilmRock(cam,-450,0,1203,94,27,54,25,24,20);
        ggFilmSofa(cam,221,1180,1);
        ggFilmTableOnFloor(cam,388,1235,99,61,65);
        ggFilmLamp(cam,388,82,1235,t);
    }else{
        ggFilmWindow(cam,413,1510,149,152,194);
        ggFilmBox(cam,-402,0,1470,124,259,54,25,24,26);
        ggFilmBox(cam,-402,251,1470,151,21,66,26,24,GG_PLASTER);
        ggFilmRoundTop(cam,-402,1440,165,101,81,24);
        ggFilmRoundTop(cam,-402,1438,172,71,63,GG_PLASTER);
        ggFilmPanel(cam,-404,-400,187,221,1436,24);
        ggFilmPanel(cam,-402,-381,196,199,1435,24);
        ggFilmPanel(cam,-441,-365,21,157,1441,24);
        int swing=((t/24)%13)-6;
        ggFilmQuad(cam,ggV3(-405,151,1438),ggV3(-399,151,1438),
                      ggV3(-399+swing,62,1438),ggV3(-404+swing,62,1438),26);
        ggFilmBox(cam,-402+swing,55,1435,27,25,10,26,25,GG_PLASTER);
        ggFilmBanner(cam,-12,1520,144,171,158,GG_OXBLOOD);
        ggFilmSofa(cam,GG_FILM_COUCH_X,GG_FILM_COUCH_Z,0);
        ggFilmBox(cam,-220,0,1167,117,72,101,25,24,26);
        ggFilmBox(cam,-220,70,1167,144,11,116,26,24,GG_PLASTER);
        ggFilmLamp(cam,-219,98,1170,t);
        ggFilmTableOnFloor(cam,24,GG_FILM_COUCH_TABLE_Z,134,GG_FILM_COUCH_TABLE_D,GG_FILM_COUCH_TABLE_TOP);
    }
}
static void ggFilmInterior(GgFilmCamera cam,int p,int t,int chapter,int winter){
    ggFilmRoomShell(cam,p);
    switch(p){
    case F_PORCH: ggFilmPorch(cam,t,chapter,winter); break;
    case F_ARCHIVE: ggFilmArchive(cam,t); break;
    case F_FORGE: ggFilmForge(cam,t); break;
    case F_HALL: case F_THRONE: ggFilmCourt(cam,p,t); break;
    case F_COUCH: case F_HEARTH: ggFilmSittingRoom(cam,p,t); break;
    }
}
// Restage only the opaque geometry of a piece of furniture when a cast
// plane is behind it. Its ground shadow still belongs to the floor pass.
static int ggFilmFurnitureFront(int place){
    switch(place){
    case F_PORCH:return GG_FILM_PORCH_TABLE_Z-GG_FILM_PORCH_TABLE_D/2;
    case F_COUCH:return GG_FILM_COUCH_TABLE_Z-GG_FILM_COUCH_TABLE_D/2;
    case F_FORGE:return GG_FILM_ANVIL_Z-GG_FILM_ANVIL_D/2;
    case F_ARCHIVE:return GG_FILM_ARCHIVE_DESK_Z-GG_FILM_ARCHIVE_DESK_D/2;
    case F_THRONE:return GG_FILM_THRONE_STAND_Z-GG_FILM_THRONE_STAND_D/2;
    default:return 0;
    }
}
static void ggFilmFurnitureForeground(GgFilmCamera cam,int place,int t){
    switch(place){
    case F_PORCH:
        ggFilmLowTable(cam,0,GG_FILM_PORCH_TABLE_Z,145,GG_FILM_PORCH_TABLE_D,GG_FILM_PORCH_TABLE_TOP);
        break;
    case F_COUCH:
        ggFilmLowTable(cam,24,GG_FILM_COUCH_TABLE_Z,134,GG_FILM_COUCH_TABLE_D,GG_FILM_COUCH_TABLE_TOP);
        break;
    case F_THRONE:
        ggFilmLowTable(cam,0,GG_FILM_THRONE_STAND_Z,118,GG_FILM_THRONE_STAND_D,GG_FILM_THRONE_STAND_TOP);
        break;
    case F_FORGE: ggFilmForgeAnvil(cam); break;
    case F_ARCHIVE: ggFilmArchiveDesk(cam,t); break;
    }
}
#endif
