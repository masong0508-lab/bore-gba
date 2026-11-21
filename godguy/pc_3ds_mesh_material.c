#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "3ds/mesh_material3d.h"
static void ok(int pass,const char*reason){
    if(!pass){fprintf(stderr,"MESH MATERIAL FAIL: %s\n",reason);exit(1);}
}
int main(void){
    int changed=0;
    for(int material=GG_MAT_MEADOW;material<=GG_MAT_FABRIC;material++){
        int min=255,max=0;
        for(int y=0;y<67;y++)for(int x=0;x<71;x++){
            int r=136,g=136,b=136;
            ggMatSample(material,x,y,0x54b3d1a2u,&r,&g,&b);
            int a=r,bb=g,c=b;
            ggMatSample(material,x,y,0x54b3d1a2u,&a,&bb,&c);
            ok(a==r&&bb==g&&c==b,
               "world-surface material sampling changes between frames");
            ok(r>=0&&r<=255&&g>=0&&g<=255&&b>=0&&b<=255,
               "material leaves 8-bit component range");
            if(r!=136||g!=136||b!=136)changed++;
            if(r<min)min=r;if(r>max)max=r;
        }
        ok(max-min>=7,"material surface is still a flat untextured polygon");
        ok(max-min<=125,"material contrast destroys readable daylight volumes");
    }
    ok(changed>35000,"material variation does not cover physical surfaces");
    int a=145,b=145,c=145;
    ggMatSample(GG_MAT_NONE,12,18,0,&a,&b,&c);
    ok(a==145&&b==145&&c==145,
       "legacy untextured geometry was unexpectedly changed");
    puts("3DS MATERIAL OK: eleven authored, world-anchored, bounded deterministic procedural surfaces");
    return 0;
}
