// 3DS lower-screen objective overlay. The original world map remains the
// canonical world; no second tile layer, texture, frame buffer, A* query,
// floating map marker, or altered 2D/DS draw function is introduced.
#ifndef GG_REIGN_MAP3D_H
#define GG_REIGN_MAP3D_H
static void ggMapPinPixel(u8 *bot,int x,int y,u8 r,u8 g,u8 b){
    if(x<32||x>=288||y<24||y>=216)return;
    u8 *p=bot+((y*320+x)*3);
    p[0]=r;p[1]=g;p[2]=b;
}
static int ggMapPinDraw(u8 *bot,int tx,int ty,u8 r,u8 g,u8 b){
    // Same eight-pixel world cells and camera offsets as the touch map.
    int x=32+tx*8+4-camX;
    int y=24+ty*8+4-camY;
    if(x<37||x>=283||y<29||y>=211)return 0;
    // 11x11 high-contrast hollow marker; leaves the world tiles visible.
    for(int k=-5;k<=5;k++){
        ggMapPinPixel(bot,x+k,y-5,r,g,b);
        ggMapPinPixel(bot,x+k,y+5,r,g,b);
        ggMapPinPixel(bot,x-5,y+k,r,g,b);
        ggMapPinPixel(bot,x+5,y+k,r,g,b);
    }
    ggMapPinPixel(bot,x,y,255,255,255);
    ggMapPinPixel(bot,x-1,y,255,255,255);
    ggMapPinPixel(bot,x+1,y,255,255,255);
    ggMapPinPixel(bot,x,y-1,255,255,255);
    ggMapPinPixel(bot,x,y+1,255,255,255);
    return 1;
}
static int ggMapObjectiveSignature(void){
    if(!ggChron.active||ggOrigin.active||ggSaga.mode)return 0;
    int q=ggQuest.stage;
    int v=q==GG_QUEST_FETCH?ggQuest.target:
          q==GG_QUEST_RETURN?ggQuest.home:-1;
    return 1+(ggChron.post+1)*131+
           q*65537+(v+1)*1021;
}
static void ggMapObjectivesDraw(u8 *bot){
    if(!ggChron.active||ggOrigin.active||ggSaga.mode)return;
    if(ggChron.post<GG_CHRON_POSTS){
        ggMapPinDraw(bot,ggChron.px[ggChron.post],
                     ggChron.py[ggChron.post],244,204,82);
    }
    int q=ggQuest.stage;
    int v=q==GG_QUEST_FETCH?ggQuest.target:
          q==GG_QUEST_RETURN?ggQuest.home:-1;
    if(v>=0&&v<VIL_N&&vil[v].on)
        ggMapPinDraw(bot,vil[v].x,vil[v].y,88,221,234);
}
#endif
