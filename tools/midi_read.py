import struct,sys
def vlq(d,p):
    v=0
    while True:
        b=d[p];p+=1;v=(v<<7)|(b&127)
        if not b&128:return v,p
def parse(path):
    d=open(path,'rb').read()
    fmt,nt,div=struct.unpack('>HHH',d[8:14]);p=14;tracks=[]
    for _ in range(nt):
        ln=struct.unpack('>I',d[p+4:p+8])[0];t=d[p+8:p+8+ln];p+=8+ln
        q=0;tick=0;ev=[];rs=0
        while q<len(t):
            dt,q=vlq(t,q);tick+=dt;b=t[q]
            if b==0xFF:
                ty=t[q+1];l,q2=vlq(t,q+2);ev.append((tick,'meta',ty,t[q2:q2+l]));q=q2+l
            elif b in(0xF0,0xF7):
                l,q=vlq(t,q+1);q+=l
            else:
                if b&128:rs=b;q+=1
                st=rs;k=st>>4
                if k in(0xC,0xD):a=t[q];q+=1;ev.append((tick,'ev',st,a,0))
                else:a,bb=t[q],t[q+1];q+=2;ev.append((tick,'ev',st,a,bb))
        tracks.append(ev)
    return div,tracks
