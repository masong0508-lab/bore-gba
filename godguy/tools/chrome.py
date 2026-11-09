import numpy as np
from PIL import Image
from logo_fix import fix
g,pal=fix()
H=len(g); W=len(g[0])
STOPS=[(0,(255,255,255)),(.22,(214,228,244)),(.44,(128,156,192)),(.50,(26,36,66)),(.56,(74,100,138)),(.72,(158,186,214)),(.90,(232,242,252)),(1,(176,200,224))]
def ramp(t):
    t=min(max(t,0),1)
    for (a,ca),(b,cb) in zip(STOPS,STOPS[1:]):
        if t<=b:
            u=(t-a)/(b-a); return tuple(ca[i]+(cb[i]-ca[i])*u for i in range(3))
    return STOPS[-1][1]
def mix(c,d,u): return tuple(c[i]+(d[i]-c[i])*u for i in range(3))
def chrome_grid():
    out=np.zeros((H,W,4),np.uint8)
    for y in range(H):
        for x in range(W):
            ch=g[y][x]
            if ch=='.': continue
            r,gg,b,_=pal[ch]
            if ch in 'b87360':
                pass
            base=ramp((y-7)/33.0)
            if ch=='b': c=base
            elif ch=='8': c=mix(base,(255,255,255),.8)
            elif ch=='7': c=mix(base,(20,30,60),.28)
            elif ch=='3': c=mix(base,(10,16,40),.55)
            elif ch=='6': c=mix(base,(6,8,26),.78)
            elif ch=='0': c=(14,10,30)
            elif ch=='5': c=(40,34,70)
            else: c=(r,gg,b)
            out[y,x]=[int(round(v)) for v in c]+[255]
    return out
if __name__=='__main__':
    o=chrome_grid(); im=Image.fromarray(o)
    bg=Image.new('RGBA',im.size,(40,40,90,255)); bg.alpha_composite(im)
    bg.resize((W*6,H*6),Image.NEAREST).convert('RGB').save('chrome_view.png')
