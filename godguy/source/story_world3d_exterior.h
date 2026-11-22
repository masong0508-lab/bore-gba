#ifndef GG_STORY_WORLD3D_EXTERIOR_H
#define GG_STORY_WORLD3D_EXTERIOR_H

static void ggFilmHouse(GgFilmCamera cam,int x,int z,int w,int h,int lit,int finished){
    int l=x-w/2,r=x+w/2,f=z-62,e=h+65;
    ggFilmGroundPatch(cam,x+36,2,z-14,w+112,205,GG_PINE);
    ggFilmBox(cam,x,0,z,w,h,124,GG_PLASTER,25,26);
    ggFilmMasonry(cam,l,r,0,48,f-1,25,43,20,GG_LIMESTONE,GG_STONE);
    // Half-timbering, plaster infill and a raised porch sill.
    ggFilmPanel(cam,l+6,r-6,49,h-8,f-2,GG_PLASTER);
    ggFilmPanel(cam,l+5,r-5,h-20,h-6,f-3,24);
    ggFilmPanel(cam,l+5,r-5,49,57,f-3,25);
    for(int i=-2;i<=2;i++){
        int sx=x+i*(w-17)/4;
        ggFilmPanel(cam,sx-4,sx+4,50,h-7,f-3,i==-2?26:24);
    }
    for(int side=-1;side<=1;side+=2){
        int a=x+side*(w/2-8),b=x+side*w/7;
        ggFilmQuad(cam,ggV3(a-4,54,f-4),ggV3(a+4,54,f-4),
                       ggV3(b+4,h-10,f-4),ggV3(b-4,h-10,f-4),25);
    }
    int roofL=l-14,roofR=r+14,roofFront=f-13,roofRear=z+75;
    ggFilmBox(cam,x+w/4,h+21,z+36,23,75,25,GG_STONE,20,GG_LIMESTONE);
    ggFilmBox(cam,x+w/4,h+94,z+36,33,8,32,20,GG_STONE,GG_LIMESTONE);
    if(finished){
        ggFilmQuad(cam,ggV3(roofL,h,roofFront),ggV3(x,e,roofFront),
                      ggV3(x,e,roofRear),ggV3(roofL,h,roofRear),GG_COPPER);
        ggFilmQuad(cam,ggV3(x,e,roofFront),ggV3(roofR,h,roofFront),
                      ggV3(roofR,h,roofRear),ggV3(x,e,roofRear),GG_OXBLOOD);
        for(int i=1;i<=4;i++){
            int y=h+i*13,dx=(w/2+14)*(65-i*13)/65;
            ggFilmQuad(cam,ggV3(x-dx,y,roofFront),ggV3(x-dx,y,roofRear),
                          ggV3(x-dx+3,y+2,roofRear),ggV3(x-dx+3,y+2,roofFront),25);
            ggFilmQuad(cam,ggV3(x+dx,y,roofFront),ggV3(x+dx,y,roofRear),
                          ggV3(x+dx-3,y+2,roofRear),ggV3(x+dx-3,y+2,roofFront),24);
        }
        ggFilmFace(cam,ggV3(roofL,h,roofFront),ggV3(x,e,roofFront),
                      ggV3(roofR,h,roofFront),25);
        ggFilmFace(cam,ggV3(l+15,h+4,roofFront-1),ggV3(x,e-12,roofFront-1),
                      ggV3(r-15,h+4,roofFront-1),26);
        ggFilmPanel(cam,x-5,x+5,h,h+49,roofFront-2,24);
        ggFilmBox(cam,x,e-2,z,13,9,152,25,GG_OXBLOOD,26);
    }else{
        // Open rafters keep the working house legible before the roof is laid.
        ggFilmBox(cam,x,e-6,z,10,10,151,26,24,27);
        for(int i=4;i>=0;i--){
            int rz=roofFront+i*32;
            ggFilmQuad(cam,ggV3(roofL,h,rz),ggV3(x,e,rz),
                          ggV3(x,e-5,rz+6),ggV3(roofL,h-5,rz+6),26);
            ggFilmQuad(cam,ggV3(x,e,rz),ggV3(roofR,h,rz),
                          ggV3(roofR,h-5,rz+6),ggV3(x,e-5,rz+6),25);
        }
    }
    ggFilmBox(cam,x,h-6,f-8,w+27,9,15,24,25,26);
    ggFilmPanel(cam,x-21,x+21,5,81,f-5,24);
    ggFilmPanel(cam,x-16,x+16,9,76,f-6,25);
    ggFilmPanel(cam,x+12,x+15,36,43,f-7,10);
    ggFilmBox(cam,x,0,f-15,56,9,30,GG_STONE,20,GG_LIMESTONE);
    for(int side=-1;side<=1;side+=2){
        int wx=x+side*w/3;
        ggFilmPanel(cam,wx-20,wx+20,83,128,f-6,24);
        ggFilmPanel(cam,wx-14,wx+14,89,122,f-7,lit?9:31);
        if(lit)ggFilmPanel(cam,wx-12,wx+11,107,121,f-8,10);
        ggFilmPanel(cam,wx-2,wx+2,87,126,f-9,25);
        ggFilmPanel(cam,wx-16,wx+16,103,107,f-9,25);
        ggFilmBox(cam,wx,81,f-13,46,6,16,25,24,26);
    }
}

static void ggFilmCrown(GgFilmCamera cam,int x,int y,int z,int w,int h,int d,
                        u8 light,u8 mid,u8 dark){
    // An irregular canopy ring wraps a raised ridge, with a quieter rear side.
    GgV3 a=ggV3(x-w/2,y+h/3,z), b=ggV3(x-w/3,y+h*2/3,z-d/3);
    GgV3 c=ggV3(x+w/12,y+h,z+d/8), e=ggV3(x+w/3,y+h*3/4,z+d/3);
    GgV3 f=ggV3(x+w/2,y+h/3,z),g=ggV3(x+w/5,y,z-d/5);
    GgV3 p=ggV3(x-w/4,y+h/7,z-d/2),m=ggV3(x-w/10,y+h*3/5,z-d/4);
    ggFilmFace(cam,a,b,m,mid);
    ggFilmFace(cam,b,c,m,light);
    ggFilmFace(cam,c,e,m,mid);
    ggFilmFace(cam,e,f,m,dark);
    ggFilmFace(cam,f,g,m,dark);
    ggFilmFace(cam,g,p,m,mid);
    ggFilmFace(cam,p,a,m,dark);
}
static void ggFilmTree(GgFilmCamera cam,int x,int z,int size,int t,int n){
    int sway=((t/31+n*5)%7)-3, offset=(n&1?1:-1)*size/15;
    ggFilmGroundPatch(cam,x+size/5,2,z-7,size*5/6,size/2,GG_PINE);
    ggFilmQuad(cam,ggV3(x-size/18,0,z),ggV3(x+size/19,0,z-5),
                  ggV3(x+offset+3,size*3/4,z+2),ggV3(x+offset-3,size*3/4,z+7),24);
    ggFilmFace(cam,ggV3(x-3,size/5,z-5),ggV3(x+4,size/3,z-5),
                  ggV3(x-size/5,size*2/3,z-9),25);
    ggFilmFace(cam,ggV3(x-2,size/5,z-4),ggV3(x+8,size/4,z-4),
                  ggV3(x+size/3,size*3/5,z+10),25);
    ggFilmCrown(cam,x+offset+sway,size/2,z+20,size*5/7,size/2,size/2,
                GG_MOSS,GG_LEAF,GG_PINE);
    ggFilmCrown(cam,x-size/4+sway,size*2/5,z-9,size*3/5,size*4/9,size/2,
                18,GG_LEAF,16);
    ggFilmCrown(cam,x+size/5+sway,size/3,z-16,size*2/3,size*5/9,size/2,
                GG_LEAF,GG_PINE,16);
}
static void ggFilmPine(GgFilmCamera cam,int x,int z,int size,int t,int n,int storm){
    int sway=((t/22+n*3)%7)-3,lean=storm?size/11:0;
    ggFilmQuad(cam,ggV3(x-6,0,z),ggV3(x+6,0,z-3),
                  ggV3(x+lean+2,size*5/6,z),ggV3(x+lean-2,size*5/6,z+3),24);
    for(int i=0;i<4;i++){
        int y=size*(2+i)/9,w=size*(5-i)/16;
        int tip=x+lean*(i+5)/8+sway*(i+3);
        GgV3 a=ggV3(x-w+lean/2,y+7,z+12),b=ggV3(x+w+lean/2,y,z+8);
        GgV3 p=ggV3(tip,y+size*4/9,z+13),c=ggV3(x+lean/2,y-12,z-w/2);
        ggFilmFace(cam,a,b,p,16);
        ggFilmFace(cam,a,c,p,storm?GG_PINE:GG_LEAF);
        ggFilmFace(cam,c,b,p,GG_PINE);
    }
}

// The west gate has real cylindrical arch depth, dressed voussoirs and
// individual jamb courses. The ruins keep the same masonry in broken piers.
static void ggFilmArch(GgFilmCamera cam,int x,int z,int broken){
    static const short archX[]={-95,-88,-67,-37,0,37,67,88,95};
    static const short archY[]={0,31,57,74,80,74,57,31,0};
    int spring=142,front=z-34;
    for(int side=-1;side<=1;side+=2){
        int sx=x+side*108,height=broken?(side<0?138:116):spring;
        ggFilmBox(cam,sx,0,z,59,height,66,GG_STONE,GG_SLATE,GG_LIMESTONE);
        ggFilmMasonry(cam,sx-29,sx+29,0,height,front-1,28,49,20,
                      GG_LIMESTONE,GG_STONE);
        ggFilmBox(cam,sx,0,z,71,18,76,GG_SLATE,20,GG_STONE);
        if(broken){
            ggFilmRock(cam,sx+8,height-8,z,63,35,59,GG_LIMESTONE,GG_STONE,GG_SLATE);
            ggFilmRock(cam,sx-side*47,0,z-22,37,25,40,GG_STONE,21,GG_SLATE);
        }
    }
    if(broken)return;
    for(int i=0;i<8;i++){
        int a=archX[i],b=archX[i+1],h=archY[i],k=archY[i+1];
        ggFilmQuad(cam,ggV3(x+a,spring+h,front),ggV3(x+b,spring+k,front),
                      ggV3(x+b,spring+k,z+30),ggV3(x+a,spring+h,z+30),20);
        ggFilmQuad(cam,ggV3(x+a,spring+h,front-1),ggV3(x+b,spring+k,front-1),
                      ggV3(x+b*121/95,spring+k*111/80,front-1),
                      ggV3(x+a*121/95,spring+h*111/80,front-1),
                      i==3||i==4?GG_LIMESTONE:i&1?GG_STONE:21);
        ggFilmFace(cam,ggV3(x+a,spring+h,front-2),
                       ggV3(x+a*121/95,spring+h*111/80,front-2),
                       ggV3(x+a+5,spring+h+2,front-2),GG_SLATE);
    }
    ggFilmBox(cam,x,224,z-34,21,36,11,GG_LIMESTONE,GG_STONE,22);
}

static void ggFilmRidge(GgFilmCamera cam,const short *height,int z,int offset,
                        u8 face,u8 shade,u8 rim){
    for(int i=0;i<10;i++){
        int x=-2750+i*550+offset,h=height[i],k=height[i+1];
        GgV3 a=ggV3(x,56,z),b=ggV3(x+550,56,z);
        GgV3 p=ggV3(x,h,z),q=ggV3(x+550,k,z);
        GgV3 fold=ggV3(x+200,(h+k)/3,z-105);
        ggFilmQuad(cam,a,b,q,p,face);
        ggFilmFace(cam,p,fold,b,shade);
        if(h>480)
            ggFilmFace(cam,p,ggV3(x-66,h-75,z),ggV3(x+168,h-114,z),rim);
    }
}
static void ggFilmClouds(GgFilmCamera cam,int storm,int evening,int tick){
    int drift=(tick/19)%72, z=3650;
    u8 shadow=storm?GG_SLATE:evening?5:GG_SKY_HIGH;
    u8 light=storm?31:evening?28:GG_CLOUD;
    static const short streaks[][4]={
        {-2230,690,1100,70}, {-1140,630,1190,53}, {-460,760,1540,73},
        {560,561,1320,48}, {840,701,1210,58},
    };
    for(unsigned i=0;i<sizeof streaks/sizeof *streaks;i++){
        int l=streaks[i][0]+drift,y=streaks[i][1],w=streaks[i][2],h=streaks[i][3];
        ggFilmQuad(cam,ggV3(l,y,z),ggV3(l+w-120,y-h/3,z),
                      ggV3(l+w,y+h/4,z),ggV3(l+w/6,y+h,z),shadow);
        ggFilmQuad(cam,ggV3(l+70,y+h/6,z-2),ggV3(l+w-60,y+h/9,z-2),
                      ggV3(l+w*4/5,y+h*2/3,z-2),ggV3(l+w/4,y+h,z-2),light);
    }
}
static void ggFilmSun(GgFilmCamera cam,int x,int y,int z,int radius,u8 color){
    static const short rim[][2]={{0,100},{59,81},{95,31},{95,-31},{59,-81},
        {0,-100},{-59,-81},{-95,-31},{-95,31},{-59,81}};
    GgS2 center=ggFilmProject(cam,ggV3(x,y,z));
    for(unsigned i=0;i<sizeof rim/sizeof *rim;i++){
        unsigned j=(i+1)%(sizeof rim/sizeof *rim);
        ggFilmTri(cam.page,center,
            ggFilmProject(cam,ggV3(x+rim[i][0]*radius/100,y+rim[i][1]*radius/100,z)),
            ggFilmProject(cam,ggV3(x+rim[j][0]*radius/100,y+rim[j][1]*radius/100,z)),color);
    }
}
static void ggFilmWater(GgFilmCamera cam,int place,int t){
    int storm=place==F_STORM,ford=place==F_FORD;
    for(int i=0;i<36;i++){
        int z=540+(i*211)%2180;
        int x=((i*317+t/(storm?4:9))%1950)-975;
        int w=36+(i*43)%186;
        if(ford&&(x>-130&&x<130))continue;
        int phase=(t/11+i*7)%17;
        u8 ink=i%5==0?GG_WATER_GLINT:i&1?GG_WATER_CREST:14;
        if(storm)ink=i%6?13:GG_STONE;
        if(i%7==0)ink=GG_WATER_DARK;
        ggFilmQuad(cam,ggV3(x,2,z),ggV3(x+w,2,z+phase),
                      ggV3(x+w-10,2,z+phase+9),ggV3(x+9,2,z+17),ink);
    }
    if(place!=F_SHORE)return;
    // The reflection lies on the sea, narrowing with distance toward the sun.
    for(int i=0;i<12;i++){
        int z=1090+i*173,x=-z*105/372+(i*67+t/16)%108-54;
        int w=104+i%3*35;
        ggFilmGroundPatch(cam,x,3,z,w,25,i%3?26:9);
    }
}
static void ggFilmBeach(GgFilmCamera cam,int place,int t){
    static const short coastX[]={-1900,-1120,-665,-360,-125,40,240,440,680,1110,1900};
    static const short coastZ[]={1110,1050,925,810,855,966,1000,981,1044,1250,1220};
    int storm=place==F_STORM;
    for(unsigned i=0;i+1<sizeof coastX/sizeof *coastX;i++){
        int l=coastX[i],r=coastX[i+1],a=coastZ[i],b=coastZ[i+1];
        int wash=(t/19+(int)i*2)%19;
        ggFilmQuad(cam,ggV3(l,2,a-9),ggV3(r,2,b-9),
                      ggV3(r,2,b+46+wash),ggV3(l,2,a+46+wash),storm?14:GG_WATER_CREST);
        ggFilmQuad(cam,ggV3(l,3,a-27),ggV3(r,3,b-27),
                      ggV3(r,3,b+15+wash),ggV3(l,3,a+15+wash),storm?GG_WATER_CREST:GG_WATER_GLINT);
        ggFilmQuad(cam,ggV3(l,3,a-74),ggV3(r,3,b-74),
                      ggV3(r,3,b),ggV3(l,3,a),storm?GG_SLATE:26);
        ggFilmQuad(cam,ggV3(l,3,330),ggV3(r,3,330),
                      ggV3(r,3,b-61),ggV3(l,3,a-61),storm?21:GG_PLASTER);
    }
    for(int i=0;i<24;i++){
        int x=(i*293+17)%1460-730,z=430+(i*239)%330;
        int w=28+i%5*13;
        ggFilmGroundPatch(cam,x,4,z,w,18+i%4*9,storm?(i%3?GG_SLATE:GG_STONE):
                        (i%3?26:GG_LIMESTONE));
        if(i%3==0&&(x<-178||x>187))ggFilmTuft(cam,x,4,z,14+(i%5)*5,storm?24:8,GG_PINE);
    }
}
static void ggFilmDryGround(GgFilmCamera cam,int place,int winter){
    int grassy=place==F_VILLAGE||place==F_ORBIT;
    // Uneven meadow. The ground stays a single walkable plane and carries
    // broad grass cuts, patches of gravel and gently curving paths.
    for(int i=0;i<16;i++){
        int x=(i*349+157)%1860-930,z=540+(i*309)%1420;
        ggFilmGroundPatch(cam,x,1,z,188+i%5*45,112+i%3*74,
                          winter?(i&1?22:23):grassy?(i%3?GG_LEAF:18):
                          (i%3?GG_SLATE:21));
    }
    if(grassy){
        static const short road[][3]={{-8,350,174},{-24,625,135},{9,845,107},
            {50,1050,99},{91,1240,76},{128,1520,57},{133,2030,33}};
        for(unsigned i=0;i+1<sizeof road/sizeof *road;i++){
            int a=road[i][0],b=road[i+1][0],n=road[i][1],f=road[i+1][1];
            int w=road[i][2],v=road[i+1][2];
            ggFilmQuad(cam,ggV3(a-w-7,2,n),ggV3(a+w+12,2,n),
                          ggV3(b+v+24,2,f),ggV3(b-v-14,2,f),winter?22:GG_MOSS);
            ggFilmQuad(cam,ggV3(a-w,3,n),ggV3(a+w,3,n),
                          ggV3(b+v,3,f),ggV3(b-v,3,f),winter?21:26);
        }
        for(int i=0;i<17;i++){
            int z=475+i*57;
            int x=(z-530)/8+((i*41)%109)-54;
            ggFilmGroundPatch(cam,x,4,z,40+i%7*7,35+i%3*13,
                              winter?(i%3?GG_STONE:22):(i%3?25:GG_PLASTER));
        }
    }
    for(int i=0;i<42;i++){
        int x=(i*303+81)%1570-785,z=424+(i*139)%1190,w=19+(i%7)*8;
        if(grassy&&x>-143&&x<216)continue;
        u8 ink=winter?(i&1?23:22):grassy?(i%3?GG_PINE:GG_MOSS):
                                    (i%3?GG_STONE:GG_LIMESTONE);
        ggFilmGroundPatch(cam,x,3,z,w,20+i%4*9,ink);
        if(i%3==0&&!winter)
            ggFilmTuft(cam,x,3,z,16+i%4*7,(i&1)?-6:5,grassy?18:GG_PINE);
    }
}
static void ggFilmSeaCliffs(GgFilmCamera cam,int p,int t){
    int storm=p==F_STORM;
    // Sloped promontories frame the low beach and leave the action lane clear.
    ggFilmRock(cam,-602,0,1050,485,207,297,storm?GG_SLATE:GG_LIMESTONE,
               storm?20:GG_STONE,GG_SLATE);
    ggFilmRock(cam,-480,0,955,350,92,170,storm?21:GG_STONE,GG_SLATE,20);
    ggFilmRock(cam,711,0,1250,482,161,281,storm?GG_SLATE:GG_LIMESTONE,21,GG_SLATE);
    ggFilmRock(cam,429,0,990,153,65,110,GG_STONE,21,GG_SLATE);
    ggFilmPine(cam,-496,1008,253,t,0,storm);
    ggFilmPine(cam,-330,1146,158,t,2,storm);
    ggFilmPine(cam,573,1213,225,t,1,storm);
    ggFilmGroundPatch(cam,-210,4,630,136,56,storm?GG_SLATE:26);
    ggFilmRock(cam,-224,3,625,103,46,63,GG_LIMESTONE,GG_STONE,GG_SLATE);
    ggFilmRock(cam,-169,3,600,41,14,66,GG_STONE,21,GG_SLATE);
    ggFilmRock(cam,277,3,701,61,25,59,GG_STONE,21,GG_SLATE);
    for(int i=0;i<8;i++){
        int x=258+i*35,z=570+(i*129)%260;
        ggFilmTuft(cam,x,3,z,31+i%3*7,storm?19:6,i%3?GG_PINE:GG_MOSS);
    }
}
static void ggFilmFence(GgFilmCamera cam,int x,int z,int n){
    for(int i=n-1;i>=0;i--){
        int wx=x+i*64;
        ggFilmBox(cam,wx,0,z,9,77,13,25,24,26);
        if(i<n-1){
            ggFilmPanel(cam,wx+4,wx+64,25,33,z-4,24);
            ggFilmPanel(cam,wx+4,wx+64,52,59,z-4,25);
        }
    }
}
static void ggFilmMarket(GgFilmCamera cam,int x,int z){
    int l=x-79,r=x+79,f=z-57;
    ggFilmGroundPatch(cam,x+34,2,z,233,176,GG_PINE);
    ggFilmBox(cam,x,0,z,155,62,94,25,24,26);
    ggFilmPanel(cam,l+10,r-10,10,54,f,24);
    for(int i=0;i<5;i++){
        int xx=l+16+i*29;
        ggFilmPanel(cam,xx,xx+21,15,49,f-1,25);
    }
    ggFilmBox(cam,l+3,60,z-39,10,102,16,26,24,27);
    ggFilmBox(cam,r-3,60,z-39,10,102,16,26,24,27);
    for(int i=0;i<7;i++){
        int xx=l-11+i*26;
        u8 ink=i&1?GG_PLASTER:GG_COPPER;
        ggFilmQuad(cam,ggV3(xx,175,z+43),ggV3(xx+26,175,z+43),
                      ggV3(xx+26,147,f-12),ggV3(xx,147,f-12),ink);
        ggFilmPanel(cam,xx,xx+26,132-(i%2)*3,147,f-12,i&1?26:GG_OXBLOOD);
    }
    ggFilmRock(cam,x-18,64,z-18,49,21,43,GG_MOSS,18,24);
    ggFilmRock(cam,x+41,64,z-25,45,25,35,8,GG_COPPER,24);
}
#endif
