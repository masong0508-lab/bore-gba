import numpy as np
from PIL import Image
SRC='/mnt/user-data/uploads/bore_logo_8x_transparent.png'
def load_grid():
    a=np.array(Image.open(SRC).convert('RGBA'))[::8,::8]
    pal={};  g=[]
    sym='0123456789abcdefghijklmnop'
    for y in range(a.shape[0]):
        r=[]
        for x in range(a.shape[1]):
            p=tuple(int(v) for v in a[y,x])
            if p[3]==0: r.append('.'); continue
            if p not in pal: pal[p]=sym[len(pal)]
            r.append(pal[p])
        g.append(r)
    return g,{v:k for k,v in pal.items()}
def remove_blunt(g):
    H=len(g); W=len(g[0])
    def setrow(y,c0,s):
        for i,ch in enumerate(s):
            if c0+i<W: g[y][c0+i]=ch
    for y in range(0,6):
        for x in range(102,W): g[y][x]='.'
    for y in range(6,15):
        for x in range(107 if y>6 else 103,W): g[y][x]='.'
    setrow(6,103,'0'*21)                       # the top outline of the E's top arm
    setrow(7,107,'3333'+'3'*10+'5000')           # top bevel, right cap, outline
    for t in range(8,14):
        hi='8888' if t==8 else '7777'
        setrow(t,107,hi+('8888bbbb' if t==8 else 'bbbbbbbb')+'bbb'[:0]+'b'+'73'+'5'+'000')
    setrow(14,107,'7777'+'7777777777'[:9]+'33'+'5'+'000')
    # the right-hand end must line up with the lower arms (face to col 120, edge 121-122, outline 123)
    return g
def fix():
    g,pal=load_grid(); g=remove_blunt(g); return g,pal
if __name__=='__main__':
    g,pal=fix()
    print('    '+''.join(str(i%10) for i in range(96,133)))
    for y in range(0,20): print('%3d '%y+''.join(g[y][96:133]))
