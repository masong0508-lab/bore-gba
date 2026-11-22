#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "3ds/mesh_material3d.h"
static void ok(int pass,const char*reason){
    if(!pass){fprintf(stderr,"MESH MATERIAL FAIL: %s\n",reason);exit(1);}
}
static int materialRed(int material,int u,int v){
    int r=136,g=136,b=136;
    ggMatSample(material,u,v,0x54b3d1a2u,&r,&g,&b);
    return r;
}
int main(void){
    int mortar=materialRed(GG_MAT_STONE,11,0);
    int stone=materialRed(GG_MAT_STONE,11,9);
    int bevel=materialRed(GG_MAT_STONE,11,14);
    ok(bevel>stone+8&&stone>mortar+12,
       "stone courses lost their lit edge or recessed mortar");
    int faint=136,green=136,blue=136;
    ggMatSampleLit(GG_MAT_STONE,11,0,0x54b3d1a2u,128,&faint,&green,&blue);
    ok(faint>mortar+5&&faint<131,
       "mortar relief does not weaken in shaded or hazy air");
    int hidden=136;green=136;blue=136;
    ggMatSampleLit(GG_MAT_STONE,11,0,0x54b3d1a2u,0,&hidden,&green,&blue);
    ok(hidden==136&&green==136&&blue==136,
       "fully attenuated relief still overpaints atmospheric color");
    int differentTimber=0;
    for(int y=-16;y<49;y++)for(int x=-32;x<33;x++)
        differentTimber+=materialRed(GG_MAT_BARK,x,y)!=
                         materialRed(GG_MAT_WOOD,x,y);
    ok(differentTimber>3000,"living bark uses the same relief as sawn boards");
    int changed=0,checked=0;
    for(int material=GG_MAT_MEADOW;material<GG_MAT_COUNT;material++){
        int min=255,max=0;
        for(int y=-18;y<86;y++)for(int x=-20;x<91;x++){
            int r=136,g=136,b=136;
            ggMatSample(material,x,y,0x54b3d1a2u,&r,&g,&b);
            // Compare identical starting colors: this sampler adds
            // material shading IN PLACE, so feeding its prior output
            // back into itself is not a determinism test.
            int a=136,bb=136,c=136;
            ggMatSample(material,x,y,0x54b3d1a2u,&a,&bb,&c);
            ok(a==r&&bb==g&&c==b,
               "world-surface material sampling changes between frames");
            ok(r>=0&&r<=255&&g>=0&&g<=255&&b>=0&&b<=255,
               "material leaves 8-bit component range");
            changed+=r!=136||g!=136||b!=136;
            checked++;
            if(r<min)min=r;
            if(r>max)max=r;
        }
        ok(max-min>=7,"material surface is still a flat untextured polygon");
        ok(max-min<=125,"material contrast destroys readable daylight volumes");
    }
    ok(changed>checked/2,"material variation does not cover physical surfaces");
    int a=145,b=145,c=145;
    ggMatSample(GG_MAT_NONE,12,18,0,&a,&b,&c);
    ok(a==145&&b==145&&c==145,
       "legacy untextured geometry was unexpectedly changed");
    printf("3DS MATERIAL OK: %d world-anchored, bounded deterministic procedural surfaces\n",
           GG_MAT_COUNT-1);
    return 0;
}
