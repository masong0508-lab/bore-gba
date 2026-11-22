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
    // Atmospheric haze shares the sky's horizon color. Let weather
    // soften the zenith before the scanline blend, with one RGB pass.
    int cover=m->daylight.cover;
    int zenithR=m->skyR+11,zenithG=m->skyG+28,zenithB=m->skyB+45;
    int horizonR=zenithR+110,horizonG=zenithG+79,horizonB=zenithB+43;
    if(cover>=0){
        zenithR=m->skyR+8+cover*12;
        zenithG=m->skyG+34+cover*8;
        zenithB=m->skyB+61+cover*3;
        horizonR=m->fogR;horizonG=m->fogG;horizonB=m->fogB;
    }
    for(int y=0;y<m->h;y++){
        int t=ggMeshClamp(y*256/m->horizon,0,256);
        int air=t*(512-t)/256;
        int r=ggMeshClamp(zenithR+(horizonR-zenithR)*air/256,0,255);
        int g=ggMeshClamp(zenithG+(horizonG-zenithG)*air/256,0,255);
        int b=ggMeshClamp(zenithB+(horizonB-zenithB)*air/256,0,255);
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

static inline void ggMeshSkyWeather(GgMeshContext *m,int frame){
    // The sun and the direct light use the same world-space direction.
    int cover=m->daylight.cover;
    int sun=m->daylight.sun;
    int forward=(GG_MESH_SUN_X*m->cosine+GG_MESH_SUN_Y*m->sine)/4096;
    int side=(-GG_MESH_SUN_X*m->sine+GG_MESH_SUN_Y*m->cosine)/4096;
    if(forward>20&&cover<3){
        int x=m->w/2+side*m->focal/forward;
        int rise=m->vertical*256*GG_MESH_SUN_Z/GG_MESH_LIGHT_HEIGHT;
        int y=m->horizon-rise/forward;
        int radius=7/m->scale;
        if(radius<2)radius=2;
        ggMeshSkyOval(m,x,y,radius*5,radius*5,246,216,171,20*sun/256);
        ggMeshSkyOval(m,x,y,radius*3,radius*3,255,228,182,36*sun/256);
        ggMeshSkyOval(m,x,y,radius+2,radius+2,255,237,190,80*sun/256);
        ggMeshSkyOval(m,x,y,radius,radius,255,247,216,244*sun/256);
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
        int sunward=(wx*GG_MESH_SUN_X+wy*GG_MESH_SUN_Y)/4096;
        int flare=ggMeshClamp(sunward,0,82)*sun/1024;
        int cr=223-tint+(m->skyR-49)/3+flare;
        int cg=227-tint+(m->skyG-78)/3+flare/2;
        int cb=222-tint/2+(m->skyB-108)/3-flare/3;
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
