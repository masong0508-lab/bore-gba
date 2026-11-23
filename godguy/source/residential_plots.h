#ifndef GG_RESIDENTIAL_PLOTS_H
#define GG_RESIDENTIAL_PLOTS_H

enum { GG_RES_EAST=1, GG_RES_SOUTH=2, GG_RES_CORNER=4 };

static inline unsigned ggResidenceMask(int x,int y,int building){
    unsigned mask=0;
    if((building==B_HOUSE_EW||building==B_ESTATE)&&x+1<WW&&
       bld[y][x+1]==B_WING_E&&bldv[y][x+1]==bldv[y][x])mask|=GG_RES_EAST;
    if((building==B_HOUSE_NS||building==B_ESTATE)&&y+1<WH&&
       bld[y+1][x]==B_WING_S&&bldv[y+1][x]==bldv[y][x])mask|=GG_RES_SOUTH;
    if(building==B_ESTATE&&x+1<WW&&y+1<WH&&bld[y+1][x+1]==B_WING_SE&&
       bldv[y+1][x+1]==bldv[y][x])
        mask|=GG_RES_CORNER;
    return mask;
}

static inline int ggResidenceResolveWing(int x,int y,int building,int *rx,int *ry){
    if(!ggBuildingIsWing(building))return B_NONE;
    int px=x-(building!=B_WING_S),py=y-(building!=B_WING_E);
    if(px<0||py<0||bldv[y][x]!=bldv[py][px])return B_NONE;
    int root=bld[py][px];
    if(root!=B_ESTATE&&!(building==B_WING_E&&root==B_HOUSE_EW)&&
       !(building==B_WING_S&&root==B_HOUSE_NS))return B_NONE;
    *rx=px;*ry=py;
    return root;
}

#endif
