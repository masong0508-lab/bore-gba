// Strict SDK-free procedural citizen identity and anatomy tests.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "3ds/procedural_people3d.h"
static void require(int ok,const char*msg){
    if(!ok){fprintf(stderr,"3DS PROCEDURAL PEOPLE FAIL: %s\n",msg);exit(1);}
}
static const unsigned char palette[3][3]={
    {19,14,10},{11,9,6},{25,20,15}
};
int main(int argc,char**argv){
    GgPersonCel a,b,c;
    unsigned int fingerprints[40]={0};
    for(int i=0;i<40;i++){
        GgPersonTraits t=ggPersonTraits((uint32_t)(i+1001),i%12);
        fingerprints[i]=t.fingerprint;
        require(t.hair<7&&t.hairTint<7&&t.style<6&&t.face<4&&
                t.build<4&&t.accessory<6&&t.complexion<8,
                "person traits escaped authored human range");
        require(t.fingerprint==
                ggPersonTraits((uint32_t)(i+1001),i%12).fingerprint,
                "person's identity changes across game frames");
        for(int j=0;j<i;j++)
            require(fingerprints[i]!=fingerprints[j],
                    "different citizens generated the same identity");
    }
    for(int turn=0;turn<8;turn++){
        ggPersonGenerate(&a,1001u,4,turn,50,0,palette);
        ggPersonGenerate(&b,1001u,4,turn,50,0,palette);
        require(!memcmp(&a,&b,sizeof a),
                "generated face, hair or clothing changes on redraw");
        unsigned int opaque=ggPersonOpaquePixels(&a);
        require(opaque>=230&&opaque<=900,
                "character anatomy collapsed or fills a rectangle");
        // Feet remain grounded, head and lower body remain inside source
        // extent, and no one is missing their silhouette from a side turn.
        int feet=0,skull=0;
        for(int x=0;x<GG_PERSON_W;x++){
            feet+=a.pixels[47*GG_PERSON_W+x]!=0;
            for(int y=0;y<17;y++)skull+=a.pixels[y*GG_PERSON_W+x]!=0;
        }
        require(feet>=8&&skull>=48,
                "profile lost planted boots or connected anatomical skull");
    }
    ggPersonGenerate(&a,1015u,6,0,50,0,palette);
    ggPersonGenerate(&b,1016u,6,0,50,0,palette);
    require(memcmp(a.pixels,b.pixels,sizeof a.pixels)!=0,
            "different citizens have indistinguishable silhouettes");
    ggPersonGenerate(&a,1015u,6,2,50,0,palette);
    ggPersonGenerate(&b,1015u,6,0,50,0,palette);
    require(memcmp(a.pixels,b.pixels,sizeof a.pixels)!=0,
            "side-profile anatomy is identical to full frontal torso");
    ggPersonGenerate(&a,1015u,6,0,50,0,palette);
    ggPersonGenerate(&b,1015u,6,4,50,0,palette);
    require(memcmp(a.pixels,b.pixels,sizeof a.pixels)!=0,
            "front/back turns are identical paper-doll cels");
    ggPersonGenerate(&a,1015u,6,0,50,0,palette);
    ggPersonGenerate(&b,1015u,6,0,135,0,palette);
    require(!memcmp(a.palette,b.palette,sizeof a.palette),
            "blinking or breathing randomized appearance and clothing");
    // Optional native source turntable image, assembled from the same
    // actual sprite generator the game draws. Not a concept mockup.
    if(argc>1){
        enum {S=3,COLS=8,ROWS=6,W=GG_PERSON_W*S*COLS,
              H=GG_PERSON_H*S*ROWS};
        unsigned char *rgb=malloc((size_t)W*H*3);
        require(rgb!=NULL,"preview export allocation failed");
        for(int i=0;i<W*H;i++){
            rgb[i*3]=38;rgb[i*3+1]=47;rgb[i*3+2]=57;
        }
        for(int r=0;r<ROWS;r++){
            for(int turn=0;turn<COLS;turn++){
                ggPersonGenerate(&c,1001u+r*11u,r+3,turn,50,0,
                                 palette);
                for(int y=0;y<GG_PERSON_H;y++)
                    for(int x=0;x<GG_PERSON_W;x++){
                        int ink=c.pixels[y*GG_PERSON_W+x];
                        if(!ink)continue;
                        int u=(turn*GG_PERSON_W+x)*S;
                        int v=(r*GG_PERSON_H+y)*S;
                        for(int dy=0;dy<S;dy++)for(int dx=0;dx<S;dx++){
                            int k=((v+dy)*W+u+dx)*3;
                            memcpy(rgb+k,c.palette[ink],3);
                        }
                    }
            }
        }
        FILE*f=fopen(argv[1],"wb");
        require(f!=NULL,"cannot create source-generated turntable");
        fprintf(f,"P6\n%d %d\n255\n",W,H);
        require(fwrite(rgb,1,sizeof(unsigned char)*(size_t)W*H*3,f)==
                (size_t)W*H*3,"cannot finish source-generated turntable");
        fclose(f);free(rgb);
    }
    puts("3DS PROCEDURAL PEOPLE OK: deterministic identity, 8 directions, consistent faces/clothing, grounded silhouettes");
    return 0;
}
