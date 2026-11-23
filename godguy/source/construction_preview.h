#ifndef GG_CONSTRUCTION_PREVIEW_H
#define GG_CONSTRUCTION_PREVIEW_H

enum { GG_PREVIEW_CLEAR_BANK=0, GG_PREVIEW_BLOCKED_BANK=1,
       GG_PREVIEW_EDGE=8, GG_PREVIEW_SHADE=9 };

// One outline per kind, with the last 0 to 7 rows cleared for the toolbar edge.
static u16 *ggBuildPreviewGfx[2][8];

static void ggBuildPreviewInit(void){
    static const char outline[2][8][9]={
        {
            ".######.",
            "#+....+#",
            "#......#",
            "#......#",
            "#......#",
            "#......#",
            "#+....+#",
            ".######."
        },
        {
            ".##..##.",
            "+......+",
            "#......#",
            "........",
            "........",
            "#......#",
            "+......+",
            ".##..##."
        }
    };

    // gfxSprPal and gfxLight only write entries 0 to 7 of each bank.
    SPRITE_PALETTE[GG_PREVIEW_CLEAR_BANK*16+GG_PREVIEW_EDGE]=RGB15(16,29,23);
    SPRITE_PALETTE[GG_PREVIEW_CLEAR_BANK*16+GG_PREVIEW_SHADE]=RGB15(5,13,10);
    SPRITE_PALETTE[GG_PREVIEW_BLOCKED_BANK*16+GG_PREVIEW_EDGE]=RGB15(31,13,10);
    SPRITE_PALETTE[GG_PREVIEW_BLOCKED_BANK*16+GG_PREVIEW_SHADE]=RGB15(15,5,4);

    for(int kind=0;kind<2;kind++){
        u16 tile[8][2]={{0}};
        for(int y=0;y<8;y++) for(int x=0;x<8;x++){
            char mark=outline[kind][y][x];
            int index=mark=='#'?GG_PREVIEW_EDGE:mark=='+'?GG_PREVIEW_SHADE:0;
            tile[y][x/4]|=(u16)(index<<((x%4)*4));
        }
        for(int y=7;y>=0;y--){
            u16 *gfx=oamAllocateGfx(&oamMain,SpriteSize_8x8,SpriteColorFormat_16Color);
            ggBuildPreviewGfx[kind][y]=gfx;
            vramCopy(gfx,tile,sizeof tile);
            tile[y][0]=tile[y][1]=0;
        }
    }
}

static void ggBuildPreviewDraw(int first,int limit,const GgBuildPlan *plan,int x,int y,int valid){
    int id=first;
    int count=plan?plan->count:0;
    int bank=valid?GG_PREVIEW_CLEAR_BANK:GG_PREVIEW_BLOCKED_BANK;
    int mapBottom=TB_Y;
    for(int i=0;i<count&&id<limit;i++){
        const GgPlanCell *cell=&plan->cell[i];
        int px=(x+cell->dx)*8-camX;
        int py=(y+cell->dy)*8-camY;
        int rows=mapBottom-py;
        if(px<=-8||px>=256||py<=-8||rows<=0) continue;
        if(rows>8) rows=8;
        int access=cell->kind==B_NONE||cell->kind==B_PAVING;
        oamSet(&oamMain,id++,px,py,0,bank,SpriteSize_8x8,SpriteColorFormat_16Color,
               ggBuildPreviewGfx[access][rows-1],-1,false,false,false,false,false);
    }
    for(;id<limit;id++) oamSetHidden(&oamMain,id,true);
}

#endif
