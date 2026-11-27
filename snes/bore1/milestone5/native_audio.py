#!/usr/bin/env python3
"""Build a resident SPC700/S-DSP arrangement from BORE 1 V14's actual title score.
SPC RAM image is shared by the native cartridge upload and standalone SPC test.
No host synthesizer is involved in playback. 100 Hz timer sequencer, 8 voices,
BRR one-shots, stereo, 64 ms DSP echo. Source ROM remains immutable.
"""
import sys,struct,json,hashlib,math,ctypes,collections
from pathlib import Path
import numpy as np
from scipy import signal
ROOT=Path(__file__).resolve().parent
BASE=0x08000000; ENTRY=0x0200; DIR=0x0500; SAMPLES=0x0600; ECHO=0xe000; HZ=100

def score(rom,track=0):
    u16=lambda p:struct.unpack_from('<H',rom,p)[0];u32=lambda p:struct.unpack_from('<I',rom,p)[0]
    t,a,x=struct.unpack_from('<III',rom,0xe8a06c+12*track);title=rom[t-BASE:rom.index(0,t-BASE)].decode();assert x and not a, title
    o,r,po,ev,vt,anc,fx,ln,data,bl,br,nord,loop,rn,rf=struct.unpack_from('<15I',rom,x-BASE)
    grid=[]
    for pat in rom[o-BASE:o-BASE+nord]:
        n=u16(r-BASE+2*pat);p=ev-BASE+u32(po-BASE+4*pat);start=len(grid)
        while len(grid)<start+n:
            h=rom[p];p+=1
            if h&128:grid.extend([[] for _ in range(h&127)]);continue
            row=[]
            for j in range(h):
                w=int.from_bytes(rom[p:p+3],'little');p+=3;v=u16(vt-BASE+2*((w&255)|((w>>22)<<8)));row.append((v&15,(v>>4)&31,(v>>9)&7,(w>>8)&127,(w>>15)&127))
            grid.append(row)
        assert len(grid)==start+n
    used=sorted({e[1] for row in grid for e in row if e[3]<96 and e[4]});rates={i:u32(anc-BASE+4*i)/2**28*18157 for i in used};samples={i:np.frombuffer(rom[u32(data-BASE+4*i)-BASE:u32(data-BASE+4*i)-BASE+u32(ln-BASE+4*i)],np.int8).astype(float)/128 for i in used};pan=np.array([list(rom[bl-BASE:bl-BASE+7]),list(rom[br-BASE:br-BASE+7])])/128;rs=(rn+rf/256)/18157
    return title,grid,rates,samples,pan,rs

class ASM:
    def __init__(self):self.b=bytearray();self.labels={};self.fix=[]
    def e(self,*b):self.b.extend(b)
    def l(self,n):assert n not in self.labels;self.labels[n]=ENTRY+len(self.b)
    def abs(self,op,n):self.e(op);self.fix.append((len(self.b),n,False));self.e(0,0)
    def rel(self,op,n):self.e(op);self.fix.append((len(self.b),n,True));self.e(0)
    def finish(self):
        for p,n,r in self.fix:
            v=self.labels[n]
            if r:
                d=v-ENTRY-p-1;assert -128<=d<=127,(n,d);self.b[p]=d&255
            else:struct.pack_into('<H',self.b,p,v)
        return self.b

def driver(seq, dictionary=None, echo=ECHO):
    a=ASM();E=a.e;L=a.l;R=a.rel;A=a.abs
    def mov(p,v):E(0x8f,v,p) # MOV dp,#imm
    def dsp(r,v):mov(0xf2,r);mov(0xf3,v)
    def pointer():mov(0x20,seq&255);mov(0x21,seq>>8)
    # Disable IPL ROM, timers, echo writes; configure DSP and enable timer.
    E(0x20,0xcd,0xef,0xbd);mov(0xf1,0);dsp(0x6c,0x60);dsp(0x5c,0xff);dsp(0x4c,0)
    for reg,val in [(0x0c,112),(0x1c,112),(0x2c,22),(0x3c,22),(0x0d,42),(0x2d,0),(0x3d,0),(0x4d,0xff),(0x5d,DIR>>8),(0x6d,echo>>8),(0x7d,(65536-echo)//2048)]:dsp(reg,val)
    for i,v in enumerate([64,32,16,8,4,0,0,0]):dsp(0x0f+i*16,v)
    for i in range(8):
        dsp(i*16+5,0);dsp(i*16+6,0);dsp(i*16+7,127)
    # Echo RAM was not part of the transfer. Explicitly zero it before enabling
    # feedback, so power-on garbage can never turn into audible noise.
    mov(0x20,0);mov(0x21,echo>>8);E(0x8d,0,0xe8,0);L('clear_echo');E(0xd7,0x20,0x3a,0x20,0xe4,0x21);R(0xd0,'clear_again');R(0x2f,'echo_done');L('clear_again');E(0xe8,0);R(0x2f,'clear_echo');L('echo_done')
    dsp(0x5c,0);dsp(0x6c,0x00);mov(0xfa,80);mov(0xf1,1);pointer();mov(0x27,0);mov(0xf4,0x42)
    # Packet [delay, keyoff mask, count, (voice-base, SRCN, pitch lo,hi,L,R)*count].
    # 0xff,0xff marker loops from start. Delay is measured against hardware timer
    # ticks, not instruction delays. First packet runs immediately.
    L('packet');A(0x3f,'get');E(0xc4,0x22);A(0x3f,'get');E(0xc4,0x23);E(0xe4,0x22,0x68,255);R(0xd0,'wait');E(0xe4,0x23,0x68,255);R(0xd0,'wait');dsp(0x5c,255);pointer();R(0x2f,'packet')
    L('wait');E(0xe4,0x22);R(0xf0,'apply');L('tick');E(0xe4,0xfd);R(0xf0,'tick');E(0xc4,0x28);A(0x3f,'mute');E(0x80,0xe4,0x22,0xa4,0x28);R(0x90,'apply');E(0xc4,0x22);R(0xd0,'tick')
    L('apply');mov(0xf2,0x5c);E(0xe4,0x23,0xc4,0xf3);A(0x3f,'get');E(0xc4,0x24);mov(0x25,0);E(0xe4,0x24);R(0xd0,'has_notes');A(0x5f,'commit');L('has_notes')
    L('note');A(0x3f,'get');E(0xc4,0x26,0x9f,0x28,7,0x5d);A(0xf5,'bits');E(0x04,0x25,0xc4,0x25)
    if dictionary is not None:
        A(0x3f,'get');E(0xc4,0x29,0x68,255);R(0xf0,'literal_note')
        for j,r in enumerate((4,2,3,0,1)):
            E(0xe4,0x26,0x60,0x88,r,0xc4,0xf2,0xe4,0x29,0x5d,0xf5,(dictionary+j*255)&255,(dictionary+j*255)>>8,0xc4,0xf3)
        R(0x2f,'note_done');L('literal_note')
    for r in (4,2,3,0,1):
        E(0xe4,0x26,0x60,0x88,r,0xc4,0xf2);A(0x3f,'get');E(0xc4,0xf3)
    if dictionary is not None:L('note_done')
    E(0x8b,0x24);R(0xf0,'commit');A(0x5f,'note')
    L('commit');mov(0xf2,0x4c);E(0xe4,0x25,0xc4,0xf3);A(0x5f,'packet')
    L('get');E(0x8d,0,0xf7,0x20,0x3a,0x20,0x6f)
    # CPU port1 bit0 mutes native music when MSU-1 is confirmed playing.
    L('mute');E(0xe4,0xf5,0x68,0x80);R(0xd0,'not_reload');dsp(0x6c,0x60);dsp(0x5c,0xff);mov(0xf1,0x80);E(0x5f,0xc0,0xff);L('not_reload');E(0xe4,0xf5,0x28,1,0x64,0x27);R(0xf0,'mutedone');E(0xc4,0x27);R(0xf0,'unmute');dsp(0x0c,0);dsp(0x1c,0);dsp(0x2c,0);dsp(0x3c,0);R(0x2f,'mutedone');L('unmute');dsp(0x0c,112);dsp(0x1c,112);dsp(0x2c,22);dsp(0x3c,22);L('mutedone');E(0x6f)
    L('bits');E(1,2,4,8,16,32,64,128)
    code=a.finish();assert ENTRY+len(code)<DIR,(len(code),hex(ENTRY+len(code)))
    return code

def encode_samples(rom,brr_dir,track=0, compact=True, echo=ECHO, spectral_reduce=False):
    title,grid,rates,samples,pan,rs=score(rom,track)
    events=[]
    for ch in range(16):
        es=[(r,e) for r,row in enumerate(grid) for e in row if e[0]==ch]
        for j,(r,(_,i,b,n,v)) in enumerate(es):
            if n>=96 or not v or i not in samples:continue
            t=r*rs;speed=rates[i]*2**((n-48)/12);end=min((es[j+1][0] if j+1<len(es) else len(grid))*rs,t+len(samples[i])/speed)
            events.append(dict(t=t,end=end,i=i,b=b,n=n,v=v,ch=ch,p=60+v*.5))
    events.sort(key=lambda e:(e['t'],e['ch']));active=[];accepted=[];steals=dropped=0
    for e in events:
        t=e['t'];active=[q for q in active if q['end']>t+1e-7]
        for q in active:
            if q['ch']==e['ch']:q['end']=t
        active=[q for q in active if q['end']>t+1e-7]
        if len(active)==8:
            old=min(active,key=lambda q:q['p']-min(22,max(0,t-q['t'])*15)-max(0,.05-(q['end']-t))*100)
            if e['p']<old['p']-min(22,max(0,t-old['t'])*15):dropped+=1;continue
            old['end']=t;active.remove(old);steals+=1
        e['voice']=next(i for i in range(8) if i not in {q['voice'] for q in active});active.append(e);accepted.append(e)
    needed={}
    for e in accepted:
        i=e['i'];n=min(len(samples[i]),math.ceil((e['end']-e['t'])*rates[i]*2**((e['n']-48)/12))+32);needed[i]=max(needed.get(i,0),n)
    reductions=[]
    if spectral_reduce:
        for i,n in sorted(needed.items()):
            x=samples[i][:n];r=rates[i]
            if len(x)<4096 or r<10000:continue
            f,p=signal.welch(x,fs=r,nperseg=min(2048,len(x)))
            energy=float(p[f>r*.215].sum()/max(1e-20,p.sum()))
            if energy>0.0005:continue
            y=signal.resample_poly(x,1,2)
            restored=signal.resample_poly(y,2,1)[:len(x)]
            error=float(np.sum((x-restored)**2)/max(1e-20,np.sum(x*x)))
            if error>0.001:continue
            reductions.append(dict(instrument=i,original_samples=len(x),new_samples=len(y),energy_above_passband=energy,relative_error=error))
            samples[i]=y;rates[i]=r/2;needed[i]=len(y)
    # Reuse encoder and original sample rate except for audited low-bandwidth instruments.
    lib=ctypes.CDLL(str(brr_dir));enc=lib.encode_brr;enc.argtypes=[ctypes.c_void_p,ctypes.c_int,ctypes.c_void_p];enc.restype=None
    ram=bytearray(65536);pos=SAMPLES;mapping={};usedbytes=0
    for i,n in sorted(needed.items()):
        x=samples[i][:n].copy();f=min(32,len(x)//3)
        if f:x[-f:]*=np.linspace(1,0,f)
        q=np.rint(np.clip(x,-.93,.93)*16384).astype(np.int16);q=np.ascontiguousarray(np.pad(q,(0,-len(q)%16)));out=np.zeros(len(q)//16*9,np.uint8);enc(q.ctypes.data,len(q),out.ctypes.data);raw=out.tobytes();assert raw[-9]&1 and not raw[-9]&2
        mapping[i]=len(mapping);struct.pack_into('<HH',ram,DIR+4*mapping[i],pos,pos);ram[pos:pos+len(raw)]=raw;pos+=len(raw);usedbytes+=len(raw)
    # Quantized 10 ms timeline; omit explicit keyoffs at natural sample ends.
    groups=collections.defaultdict(lambda:[0,[]]);by_voice=[[] for _ in range(8)]
    for e in accepted:by_voice[e['voice']].append(e)
    for voice,es in enumerate(by_voice):
        for j,e in enumerate(es):
            t=round(e['t']*HZ);end=max(t+1,round(e['end']*HZ));p=max(1,min(16383,round(rates[e['i']]/32000*4096*2**((e['n']-48)/12))));l,r=(round(min(127, e['v']*pan[k,e['b']]*.68)) for k in (0,1));groups[t][1].append(bytes([voice*16,mapping[e['i']],p&255,p>>8,l,r]));natural=(needed[e['i']]/(rates[e['i']]*2**((e['n']-48)/12)))
            nxt=round(es[j+1]['t']*HZ) if j+1<len(es) else 10**9
            if end<nxt and (e['end']-e['t'])<natural-.015:groups[end][0]|=1<<voice
    duration=round(len(grid)*rs*HZ);groups[duration+100][0]=255
    # An exact dictionary preserves all chosen pitch and pan/volume tuples.
    # The dictionary is transposed to avoid multiplication on SPC700.
    templates=[x[1:] for _,starts in groups.values() for x in starts]
    dictionary=[k for k,v in collections.Counter(templates).most_common(255)] if compact else []
    indices={k:i for i,k in enumerate(dictionary)}
    table=b''.join(bytes(k[j] for k in dictionary).ljust(255,b'\0') for j in range(5)) if compact else b''
    stream=bytearray();plain=bytearray();prev=0;merged=0
    for t,(off,starts) in sorted(groups.items()):
        delta=t-prev
        while delta>254:stream.extend([254,0,0]);plain.extend([254,0,0]);delta-=254
        unique={e[0]:e for e in starts};merged+=len(starts)-len(unique);starts=list(unique.values());assert len(starts)<=8
        for e in starts:off&=~(1<<(e[0]//16))
        plain.extend([delta,off,len(starts)]);plain.extend(b''.join(starts))
        stream.extend([delta,off,len(starts)])
        for event in starts:
            if not compact:stream.extend(event);continue
            k=indices.get(event[1:],255);stream.extend([event[0],k]);
            if k==255:stream.extend(event[1:])
        prev=t
    stream.extend([255,255]);plain.extend([255,255])
    if compact:
        # Independent lossless check of every event packet.
        out=bytearray();p=0
        while p<len(stream):
            d,o=stream[p:p+2];p+=2;out.extend([d,o])
            if d==o==255:break
            c=stream[p];p+=1;out.append(c)
            for _ in range(c):
                v,k=stream[p:p+2];p+=2;out.append(v)
                if k==255:out.extend(stream[p:p+5]);p+=5
                else:out.extend(table[k+j*255] for j in range(5))
        assert p==len(stream) and out==plain
    dicpos=pos;seq=pos+len(table);end=seq+len(stream)
    print(title,len(accepted),'notes',len(stream),'score bytes',usedbytes,'BRR bytes',hex(end),flush=True);assert end<=echo,(hex(end),len(stream))
    ram[dicpos:seq]=table;ram[seq:end]=stream;code=driver(seq,dicpos if compact else None,echo);ram[ENTRY:ENTRY+len(code)]=code
    rep=dict(title=title,events=len(events),accepted=len(accepted),dropped=dropped,voice_steals=steals,same_tick_replaced=merged,score_bytes=len(stream)+len(table),uncompressed_score_bytes=len(plain),dictionary_entries=len(dictionary),spectral_reductions=reductions,brr_bytes=usedbytes,driver_bytes=len(code),aram_end=end,echo_bytes=65536-echo,aram_free=echo-end,duration_seconds=duration/HZ,source_sha256=hashlib.sha256(rom).hexdigest())
    return ram,end,rep

def make_spc(ram,title='The Dipper Man',duration=171):
    h=bytearray(256);h[:33]=b'SNES-SPC700 Sound File Data v0.30';h[0x21:0x25]=bytes([26,26,26,30]);struct.pack_into('<H',h,0x25,ENTRY);h[0x2b]=0xef;t=title.encode('ascii','replace')[:32];h[0x2e:0x2e+len(t)]=t;h[0x4e:0x4e+6]=b'BORE 1';h[0xa9:0xac]=f'{min(999,round(duration)):03d}'.encode();h[0xac:0xb1]=b'01000';dsp=bytearray(128);dsp[0x6c]=0x60
    return bytes(h)+bytes(ram)+bytes(dsp)+bytes(128)

if __name__=='__main__':
    r=Path(sys.argv[1]).read_bytes();ram,end,report=encode_samples(r,Path(sys.argv[2]));(ROOT/'native_title.bin').write_bytes(ram[ENTRY:end]);(ROOT/'native_title.spc').write_bytes(make_spc(ram));(ROOT/'audio_report.json').write_text(json.dumps(report,indent=2));print(report)
