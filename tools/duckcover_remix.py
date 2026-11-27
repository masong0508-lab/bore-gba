#!/usr/bin/env python3
"""Generate the visible Duck & Cover remix from the existing compiled XM bank.

Share every original PCM sample, pitch anchor and length with the hidden
original. Preserve the lyric and bass note onsets/pitches. Rebuild the rhythm,
stereo responses, dynamics and ending without another full music asset.
"""
import re
import collections
import hashlib
from pathlib import Path
P='xm_duck_cover_dnb_'

def decode_stream(stream, offsets, rows, order, vtab):
    grid=[]
    for pi in order:
        pos=offsets[pi]; out=[]
        while len(out)<rows[pi]:
            count=stream[pos]; pos+=1
            if count&128:
                assert count&127
                out.extend([] for _ in range(count&127)); continue
            events=[]
            for _ in range(count):
                w=int.from_bytes(stream[pos:pos+3],'little');pos+=3
                v=vtab[(w&255)|((w>>22)<<8)]
                events.append((v&15,(v>>4)&31,(v>>9)&7,(w>>8)&127,(w>>15)&127))
            out.append(events)
        assert len(out)==rows[pi]
        grid.extend(out)
    return grid

def remix(grid):
    assert len(grid)==2560
    result=[{} for _ in grid]
    counts=collections.Counter()
    def put(r,ch,ins,bus,n,v):
        if 0<=r<len(result): result[r][ch]=(ch,ins,bus,n,max(0,min(127,int(v))))
    def vocals(g):
        return [(r,e[0],e[1],e[3]) for r,row in enumerate(g) for e in row if 8<=e[1]<=19]
    orig=vocals(grid)
    for r,row in enumerate(grid):
        b=r//32;s=r%32
        for ch,ins,bus,n,v in row:
            if ins in (0,1,2,4): continue
            if ins in (6,7):
                scale=(.90 if ins==6 else .83)*(1.06 if s%8==0 else .91 if s%8 in (3,7) else 1)
                if 23<=b<27 or 45<=b<49: scale*=.76
                if 73<=b<77: scale*=max(.40,1-(b-73)*.16)
                put(r,ch,ins,3 if ins==6 else (2 if b%2 else 4),n,v*scale)
                counts['bass_attacks']+=1
            elif ins==5:
                scale=.84 if b<8 or 23<=b<27 else .96
                put(r,ch,ins,2 if ch%2==0 else 4,n,v*scale)
            elif 8<=ins<=19:
                put(r,ch,ins,3,n,min(127,v*1.09));counts['vocal_attacks']+=1
            else: put(r,ch,ins,2 if ins==20 else 4 if ins==21 else 3,n,v*.85)
    def drum(b,s,ch,ins,v,bus=3,n=48):
        r=b*32+s
        if r<len(result) and ch not in result[r]:
            put(r,ch,ins,bus,n,v);counts['new_drum_attacks']+=1
    for b in range(77):
        if b<4: kicks=[0] if b%2==0 else [0,22]; snares=[16] if b>=2 else []; hats=[4,12,20,28]; ghost=[]
        elif b<8: kicks=[0,11,22] if b%2 else [0,12,20]; snares=[8,24]; hats=list(range(0,32,4));ghost=[19,30]
        elif 23<=b<27 or 45<=b<49: kicks=[0,12,23] if b%2 else [0,20]; snares=[16]; hats=[0,6,12,20,26,30];ghost=[29]
        elif 73<=b: kicks=[0,20] if b<76 else [0]; snares=[8,24] if b<76 else [16];hats=list(range(0,32,8));ghost=[]
        else:
            kicks=[[0,12,20],[0,18,22],[0,10,20,28],[0,14,20]][b%4]
            snares=[8,24];hats=list(range(0,32,4));ghost=[14,30] if b%2 else [6,19]
            if b>=56: hats+= [6,14,22,30];ghost+= [27] if b%4==3 else []
        for s in kicks:drum(b,s,0,0,66 if s==0 else 53)
        for s in snares:drum(b,s,1,1,62 if s in (8,16) else 59)
        for s in ghost:drum(b,s,1,1,16+(s%3)*3,3,47)
        for s in sorted(set(hats)):
            drum(b,s,2,2,34 if s%8==0 else 24 if s%4==0 else 16,2 if (s//4+b)%2 else 4,48+(1 if s%8==4 else 0))
        if b in (0,4,8,19,23,27,31,41,45,49,56,64,72,76): drum(b,0,3,4,35 if b in (0,23,45) else 44,2 if b%2 else 4)
        if b in (7,22,26,44,48,55,71,75):
            for s in (26,28,30,31):drum(b,s,1,1,20+(s-26)*5,3,47 if s<30 else 48)
    used={e[0] for row in grid for e in row}; free=[c for c in (15,14,13) if c not in used]
    if free:
        c=free[0]; last=-100
        for r,row in enumerate(grid):
            b=r//32
            if not (8<=b<73) or b in range(23,27) or b in range(45,49):continue
            for ch,ins,bus,n,v in row:
                if ins==5 and r%8==0 and r-last>=8 and r+6<len(result):
                    put(r+3,c,5,5 if ch%2==0 else 1,n,max(5,v*.25))
                    put(r+6,c,5,3,96,0)
                    last=r;counts['echo_attacks']+=1;break
    result=[sorted(row.values()) for row in result]
    assert vocals(result)==orig
    assert [(r,e[:2],e[3]) for r,row in enumerate(grid) for e in row if e[1] in (6,7)]==[(r,e[:2],e[3]) for r,row in enumerate(result) for e in row if e[1] in (6,7)]
    assert all(len({e[0] for e in row})==len(row)<=16 for row in result)
    return result,dict(counts)

def encode(grid):
    vtab=[]; mapping={}; offsets=[]; stream=bytearray()
    for begin in range(0,len(grid),64):
        offsets.append(len(stream)); run=0
        for row in grid[begin:begin+64]:
            if not row: run+=1;continue
            if run:stream.append(128|run);run=0
            stream.append(len(row))
            for ch,ins,bus,n,v in row:
                key=ch|(ins<<4)|(bus<<9)
                if key not in mapping:mapping[key]=len(vtab);vtab.append(key)
                vi=mapping[key];assert vi<1024
                w=(vi&255)|(n<<8)|(v<<15)|((vi>>8)<<22);stream.extend(w.to_bytes(3,'little'))
        if run:stream.append(128|run)
    assert decode_stream(stream,offsets,[64]*len(offsets),range(len(offsets)),vtab)==grid
    return stream,offsets,vtab

def build(root):
    root=Path(root); h=(root/'source/musicdata.h').read_text()
    def arr(name):
        m=re.search(r'\b'+P+name+r'\[\d+\]\s*=\s*\{([^}]+)\}',h);assert m,name
        return [int(x) for x in re.findall(r'\d+',m[1])]
    m=re.search(r'\.incbin \\"source/music/xmdata\.bin\\",(\d+),(\d+)\\n"',h[h.index('".global '+P+'ev'):])
    if not m: raise ValueError('Duck & Cover original event bank not found')
    start,n=map(int,m.groups());bank=(root/'source/music/xmdata.bin').read_bytes();original=bank[start:start+n]
    grid=decode_stream(original,arr('patOff'),arr('rows'),arr('order'),arr('vt'))
    revised,stats=remix(grid)
    # Recover exact converted samples, without changing the existing bank.
    import numpy as np
    from duckcover_vocal_pass import vocal_edit
    names=re.search(r'\b'+P+r'data\[\d+\]\s*=\s*\{([^}]+)\}',h)[1].split(',')
    names=[v.strip() for v in names]
    original_lens=arr('len')
    samples=[]
    for i,name in enumerate(names):
        if name=='0':
            samples.append(np.zeros(0,dtype=np.int8));continue
        marker='".global '+name+'\\n'
        m=re.search(r'\.incbin \\"source/music/xmdata\.bin\\",(\d+),(\d+)\\n"',h[h.index(marker):])
        pos,size=map(int,m.groups())
        assert size>=original_lens[i]+1
        samples.append(np.frombuffer(bank[pos:pos+original_lens[i]],dtype=np.int8))
    revised,assets,anchors,vocal_stats=vocal_edit(revised,samples,arr('anc'))
    stream,off,vt=encode(revised)
    assert len(stream)<16000 and len(anchors)<=32
    lens=original_lens+[0]*(len(anchors)-len(original_lens))
    pointers=names+['0']*(len(anchors)-len(names))
    pcm=bytearray()
    for i,x in sorted(assets.items()):
        assert len(x)>1 and x[-1]==0
        pointers[i]='dc_vocal_pcm+'+str(len(pcm))
        lens[i]=len(x)-1
        pcm.extend(x.tobytes())
    assert len(pcm)<500000
    def write_changed(path,value):
        if not path.exists() or path.read_bytes()!=value:path.write_bytes(value)
    write_changed(root/'source/music/duckcover_vocals.bin',bytes(pcm))
    def a(typ,name,values):return 'static const '+typ+' dc_remix_'+name+'[]={\n'+''.join(','.join(str(x) for x in values[i:i+24])+',\n' for i in range(0,len(values),24))+'};\n'
    text='// Generated Duck & Cover vocal remix; original remains unchanged.\n'
    text+='__asm__(".pushsection .rodata\\n.balign 4\\n.global dc_vocal_pcm\\ndc_vocal_pcm:\\n.incbin \\"source/music/duckcover_vocals.bin\\"\\n.popsection\\n");\nextern const s8 dc_vocal_pcm[];\n'
    text+=a('u8','ev',list(stream))+a('u32','patOff',off)+a('u16','vt',vt)+a('u32','anc',anchors)+a('u32','len',lens)
    text+='static const s8*const dc_remix_data[]={'+','.join(pointers)+'};\n'
    text+='static const XmSong xm_duck_cover_remix={'+','.join([P+'order',P+'rows','dc_remix_patOff','dc_remix_ev','dc_remix_vt','dc_remix_anc',P+'fx','dc_remix_len','dc_remix_data',P+'busL',P+'busR','40','0','782','161'])+'};\n'
    write_changed(root/'source/duckcover_remix.h',text.encode('utf8'))
    print('Duck & Cover remix:',stats,'vocal edit:',vocal_stats,'event bytes',len(stream),'new PCM bytes',len(pcm),'original unchanged',hashlib.sha256(original).hexdigest()[:12])
    return dict(stats,**vocal_stats)
if __name__=='__main__':build(Path(__file__).resolve().parent.parent)
