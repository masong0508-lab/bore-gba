// Native sky painting for GodGuy's existing RGB target. Included after
// GgMeshContext: all spans use the active viewport scale and never touch
// world depth. No texture, scratch image, floating point or heap storage.
#ifndef GG_MESH_SKY3D_H
#define GG_MESH_SKY3D_H

static inline void ggMeshSkySpan(GgMeshContext *m,int y,int x0,int x1,
                                 int r,int g,int b,int alpha){
    if((unsigned)y>=(unsigned)m->h||x1<0||x0>=m->w)return;
    x0=ggMeshClamp(x0,0,m->w-1);x1=ggMeshClamp(x1,0,m->w-1);
    r=ggMeshClamp(r,0,255);g=ggMeshClamp(g,0,255);
    b=ggMeshClamp(b,0,255);alpha=ggMeshClamp(alpha,0,256);
    for(int x=x0;x<=x1;x++){
        unsigned char *p=m->rgb+(y*m->scale*400+x*m->scale)*3;
        int rr=(p[0]*(256-alpha)+r*alpha)>>8;
        int gg=(p[1]*(256-alpha)+g*alpha)>>8;
        int bb=(p[2]*(256-alpha)+b*alpha)>>8;
        for(int dy=0;dy<m->scale;dy++)for(int dx=0;dx<m->scale;dx++){
            unsigned char *q=p+dy*400*3+dx*3;
            q[0]=(unsigned char)rr;q[1]=(unsigned char)gg;
            q[2]=(unsigned char)bb;
        }
    }
}

static inline void ggMeshSkyFill(GgMeshContext *m){
    // A broad blue zenith opens into a pale warm horizon. Compute the
    // curve once per scanline, rather than repeating a shader per pixel.
    for(int y=0;y<m->h;y++){
        int t=ggMeshClamp(y*256/m->horizon,0,256);
        int air=t*(512-t)/256;
        int r=ggMeshClamp(m->skyR+11+110*air/256,0,255);
        int g=ggMeshClamp(m->skyG+28+79*air/256,0,255);
        int b=ggMeshClamp(m->skyB+45+43*air/256,0,255);
        unsigned char *p=m->rgb+y*m->scale*400*3;
        for(int x=0;x<400;x++){
            p[x*3]=(unsigned char)r;p[x*3+1]=(unsigned char)g;
            p[x*3+2]=(unsigned char)b;
        }
        if(m->scale==2)memcpy(p+400*3,p,400*3);
    }
}

static inline void ggMeshSkyOval(GgMeshContext *m,int cx,int cy,
                                 int rx,int ry,int r,int g,int b,int alpha){
    if(rx<1||ry<1||cx+rx<0||cx-rx>=m->w||
       cy+ry<0||cy-ry>=m->h)return;
    int y0=ggMeshClamp(-ry,-cy,m->h-1-cy);
    int y1=ggMeshClamp(ry,-cy,m->h-1-cy);
    for(int dy=y0;dy<=y1;dy++){
        int reach=rx;
        // Small authored puffs only; the ellipse stays below 90x30px.
        while(reach>0&&reach*reach*ry*ry+dy*dy*rx*rx>rx*rx*ry*ry)
            reach--;
        ggMeshSkySpan(m,cy+dy,cx-reach,cx+reach,r,g,b,alpha/3);
        if(reach>1)ggMeshSkySpan(m,cy+dy,cx-reach+1,cx+reach-1,
                                r,g,b,alpha);
    }
}

static inline void ggMeshSkyWeather(GgMeshContext *m,int frame,int cover){
    // Sun direction matches the physical polygon light (-70,+42,+112).
    // It moves across the image when the player turns, not with the HUD.
    int forward=(-70*m->cosine+42*m->sine)/4096;
    int side=(70*m->sine+42*m->cosine)/4096;
    if(forward>20&&cover<3){
        int x=m->w/2+side*m->focal/forward;
        int y=m->horizon-(m->h*11/60)*82/forward;
        int radius=7/m->scale;
        if(radius<2)radius=2;
        ggMeshSkyOval(m,x,y,radius*5,radius*5,242,211,157,13);
        ggMeshSkyOval(m,x,y,radius*3,radius*3,255,226,171,24);
        ggMeshSkyOval(m,x,y,radius+2,radius+2,255,232,184,68);
        ggMeshSkyOval(m,x,y,radius,radius,255,245,212,244);
    }
    // Sixteen directions describe one continuous sky dome. Interpolated
    // angular wind wraps without a reset/pop; no camera-position grid
    // anchor can make a cloud jump when the player crosses a map cell.
    static const short dir[16][2]={
        {4096,0},{3784,1567},{2896,2896},{1567,3784},
        {0,4096},{-1567,3784},{-2896,2896},{-3784,1567},
        {-4096,0},{-3784,-1567},{-2896,-2896},{-1567,-3784},
        {0,-4096},{1567,-3784},{2896,-2896},{3784,-1567}
    };
    int wind=(int)(((uint32_t)frame/32u)&4095u);
    cover=ggMeshClamp(cover,0,3);
    for(int bank=0;bank<16;bank++){
        if(cover==0&&(bank&1))continue;
        int phase=(bank*256+wind)&4095;
        int at=phase>>8,next=(at+1)&15,t=phase&255;
        int wx=(dir[at][0]*(256-t)+dir[next][0]*t)/256;
        int wy=(dir[at][1]*(256-t)+dir[next][1]*t)/256;
        int f=(wx*m->cosine+wy*m->sine)/4096;
        int s=(-wx*m->sine+wy*m->cosine)/4096;
        if(f<1800)continue;
        int cx=m->w/2+s*m->focal/f;
        int cy=(25+(bank*23)%57)/m->scale;
        int width=(31+(bank*11)%20)/m->scale;
        int height=(7+(bank*7)%6)/m->scale;
        if(height<2)height=2;
        int tint=cover*4;
        int cr=223-tint+(m->skyR-49)/3;
        int cg=227-tint+(m->skyG-78)/3;
        int cb=222-tint/2+(m->skyB-108)/3;
        int opacity=cover==3?230:206;
        // Cool, wide undersides support overlapping warm lit shoulders.
        // Each bank has only five small clipped ovals at either quality.
        ggMeshSkyOval(m,cx,cy+height/2,width,height/2+1,
                      cr-35,cg-27,cb-13,opacity-45);
        ggMeshSkyOval(m,cx-width/3,cy,width*2/3,height,
                      cr-10,cg-6,cb+1,opacity);
        ggMeshSkyOval(m,cx+width/4,cy-height/3,width/2,height+1,
                      cr,cg,cb,opacity);
        ggMeshSkyOval(m,cx+width*2/3,cy+height/3,width/3,height*2/3,
                      cr-17,cg-12,cb-2,opacity);
        ggMeshSkyOval(m,cx,cy-height/2,width/3,height/2,
                      cr+12,cg+9,cb+5,opacity-26);
    }
}
#endif
