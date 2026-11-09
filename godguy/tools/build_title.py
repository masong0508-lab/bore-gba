import numpy as np, sys
from PIL import Image, ImageDraw, ImageFont, ImageFilter, ImageChops
from chrome import chrome_grid
FONT=sys.argv[1] if len(sys.argv)>1 else '/usr/share/fonts/opentype/noto/NotoSansCJK-Black.ttc'
SS=6; CW,CH=256,84            # final canvas; supersample factor
# ---------- GODGUY: heavy slab letters, extruded like the Life of Brian title
def text_layer():
    W,H=CW*SS,CH*SS
    f=ImageFont.truetype(FONT,int(46*SS),index=0)
    word='GODGUY'; track=int(1.5*SS)
    ws=[f.getlength(c) for c in word]; tot=sum(ws)+track*(len(word)-1)
    m=Image.new('L',(W,H),0); d=ImageDraw.Draw(m)
    x=(W-tot)/2; base=int(60*SS)
    for c,w in zip(word,ws):
        d.text((x,base),c,font=f,fill=255,anchor='ls'); x+=w+track
    # heavier, squarer: fatten a touch
    m=m.filter(ImageFilter.MaxFilter(SS|1))
    return m
def persp(im,coef,resample=Image.BICUBIC): return im.transform(im.size,Image.PERSPECTIVE,coef,resample)
def find_coef(src,dst):
    A=[];B=[]
    for (x,y),(u,v) in zip(dst,src):
        A+= [[x,y,1,0,0,0,-u*x,-u*y],[0,0,0,x,y,1,-v*x,-v*y]]; B+=[u,v]
    return np.linalg.solve(np.array(A,float),np.array(B,float)).tolist()
def render_text():
    m=text_layer(); W,H=m.size
    # perspective: the word climbs to the right and shrinks a little (vanishing point right), like the poster
    src=[(0,0),(W,0),(W,H),(0,H)]
    k=SS
    dst=[(2*k,16*k),(W-4*k,0*k),(W-4*k,H-16*k),(2*k,H+0*k)]
    # extrusion first, in flat space, then warp everything together
    depth=int(13*SS); dx,dy=0.80,0.62
    face=Image.new('RGBA',(W,H),(0,0,0,0))
    arr=np.array(m)>127
    out=np.zeros((H,W,4),np.float32)
    def put(mask,col):
        out[mask]=col
    # side walls, back to front
    for i in range(depth,0,-1):
        sh=ImageChops.offset(m,int(i*dx),int(i*dy))
        mk=np.array(sh)>127
        u=i/depth
        col=np.array([int(150-95*u),int(66-40*u),int(10-6*u),255],np.float32)   # warm orange -> deep brown with depth
        put(mk,col)
    # dark outline behind everything
    ol=np.array(ImageChops.offset(m,int(depth*dx),int(depth*dy)).filter(ImageFilter.MaxFilter(2*SS+1)))>127
    ol_only=ol&(out[...,3]==0)
    out[ol_only]=[18,6,4,255]
    # also outline around the face
    fo=np.array(m.filter(ImageFilter.MaxFilter(SS+1)))>127
    fo_only=fo&~arr
    out[fo_only]=[22,8,4,255]
    # the front face: gold gradient, bright top-left bevel, darker bottom/right bevel
    ys,xs=np.nonzero(arr)
    y0,y1=ys.min(),ys.max()
    t=(np.arange(H)-y0)/max(1,(y1-y0))
    top=np.array([255,236,120]); mid=np.array([255,176,30]); bot=np.array([226,110,10])
    def grad(tt):
        tt=np.clip(tt,0,1)[:,None]
        return np.where(tt<.5,top+(mid-top)*(tt/.5),mid+(bot-mid)*((tt-.5)/.5))
    G=grad(t)   # H x 3
    face_rgb=np.repeat(G[:,None,:],W,axis=1)
    # bevel: distance from the edge via repeated erosion
    bev=int(2.2*SS)
    inner=np.array(m.filter(ImageFilter.MinFilter(bev*2+1)))>127
    edge=arr&~inner
    # light from top-left: edge pixels whose up-left neighbour is outside are bright, down-right dark
    ul=np.array(ImageChops.offset(m,bev,bev))>127           # shifted down-right: where mask is present at (x-bev,y-bev)
    dr=np.array(ImageChops.offset(m,-bev,-bev))>127
    hi=arr&~ul   # nothing up-left of me -> top-left rim
    lo=arr&~dr
    f=face_rgb.copy()
    f[hi]=f[hi]*0.25+np.array([255,252,215])*0.75
    f[lo&~hi]=f[lo&~hi]*0.62
    out[arr,:3]=f[arr]; out[arr,3]=255
    img=Image.fromarray(out.astype(np.uint8),'RGBA')
    coef=find_coef(src,dst)
    # warp colour premultiplied so edges stay clean
    a=img.split()[3]; rgb=Image.merge('RGB',img.split()[:3])
    rgb=persp(rgb,coef); a=persp(a,coef,Image.BILINEAR)
    img=Image.merge('RGBA',(*rgb.split(),a))
    return img
def compose():
    big=render_text()
    small=big.resize((CW,CH),Image.BOX)
    sa=np.array(small); alpha=sa[...,3]
    # BORE chrome, 3/4 scale
    c=Image.fromarray(chrome_grid(),'RGBA')
    bw,bh=100,34
    pm=np.array(c).astype(np.float32); pm[...,:3]*=pm[...,3:]/255.
    pim=Image.fromarray(pm.astype(np.uint8),'RGBA')
    r=np.array(pim.resize((bw,bh),Image.BOX)).astype(np.float32)
    a=r[...,3]; rgb=np.where(a[...,None]>0,r[...,:3]*255./np.maximum(a[...,None],1),0)
    bore=np.dstack([np.clip(rgb,0,255),a]).astype(np.uint8)
    canvas=np.zeros((CH,CW,4),np.uint8)
    # hard alpha for the DS: coverage >= 50%
    t=sa.copy(); hard=t[...,3]>=128
    # un-premultiply is not needed: BOX of straight RGBA averaged transparent black into the edges, so recover colour with alpha weighting
    sm=np.array(big.resize((CW,CH),Image.BOX)).astype(np.float32)
    canvas[hard]=np.dstack([np.clip(sm[...,:3]*255./np.maximum(sm[...,3:],1)* (sm[...,3:]/255.)*0+sm[...,:3],0,255),np.full((CH,CW),255)]).astype(np.uint8)[hard]
    # BORE at the top-left, in front
    ox,oy=4,0
    bh_mask=bore[...,3]>=128
    sub=canvas[oy:oy+bh,ox:ox+bw]; sub[bh_mask]=bore[bh_mask]; sub[bh_mask,3]=255
    return canvas
if __name__=='__main__':
    cv=compose(); Image.fromarray(cv).save('title_rgba.png')
    bg=Image.new('RGBA',(CW,CH),(5,8,24,255)); bg.alpha_composite(Image.fromarray(cv))
    bg.resize((CW*4,CH*4),Image.NEAREST).convert('RGB').save('title_view.png')
