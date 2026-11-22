#!/usr/bin/env python3
"""Import original Koala Sampler Winner project directly to BORE 1's 16-channel XM jukebox.

Unlike the older stem transcription, this reconstructs the seven populated song-mode
sequences and ORIGINAL recorded sample pads from Winner.koala. No guessed pitches.

Usage: python3 tools/import_winner_koala.py Winner.koala
       python3 tools/xm2gba.py
Source uses numpy, scipy, soundfile (also used by other BORE importers).
XM player restrictions: 16 tracks, 32 instruments, mono PCM samples, 8-bit
samples, no effects/panning/envelopes/note-off; playback approximates Koala FX.
"""
import argparse
import collections
import io
import json
import math
import os
import pathlib
import struct
import zipfile

import numpy as np
from scipy import signal
import soundfile as sf

ROWS_PER_BEAT = 16
ROWS_PER_BAR = ROWS_PER_BEAT * 4
XM_CHANNELS = 16
XM_SPEED = 2
XM_BPM = 170  # 29.412 ms/row, 0.045% timing error vs 127.565 BPM Koala
INPUT_RATE = 48000
OUTPUT_RATE = 16000  # preserves upper presence up to 8k, near the 18,157 Hz GBA mixer limit
STOP_NAME = 'KOALA GATE'


def poly_key(note):
    return (int(note['num']), note['timeOffset'], -float(note.get('vel', 110)), float(note.get('pitch', 0)))


def beat_to_row(beat, swing):
    """Koala swing is applied at playback; timestamps themselves are unswung.
    Piecewise eighth-note ratio moves the beat midpoint from 0.5 to swing.
    16 tracker rows/beat preserve all 1/8-grid subdivisions and a 29ms bound.
    """
    whole = math.floor(beat)
    frac = beat - whole
    local = (frac * (swing / .5)) if frac < .5 else (swing + (frac - .5) * ((1. - swing) / .5))
    return int(round((whole + local) * ROWS_PER_BEAT))


def delta8(q):
    a=np.asarray(q,dtype=np.int16)
    prev=np.concatenate(([0],a[:-1]))
    return ((a-prev) & 255).astype(np.uint8).tobytes()


def sample_header(sample, name, relative, fine, vol):
    """40-byte FastTracker II signed 8-bit sample header + delta-PCM body."""
    sh=struct.pack('<IIIBbBBbB',len(sample),0,0,vol,fine,0,128,relative,0)
    return sh + name.encode('latin-1','replace')[:22].ljust(22,b'\0')


def instrument_bytes(name, pcm, pitch=0.0, vol=64):
    if pcm is None or len(pcm)==0:pcm=np.zeros(8,dtype=np.int8)
    sample=np.asarray(pcm,dtype=np.int8)
    # XM audio at sample note 49 should play at ~16kHz. The relative note and
    # finetune set the true source pitch; Koala pad pitch is folded in here.
    semis=12*math.log2(OUTPUT_RATE/8363) + pitch
    rel=math.floor(semis+.5)
    fine=int(round(128*(semis-rel)))
    if fine>=128:rel+=1;fine-=128
    if fine < -128:rel-=1;fine+=128
    assert -128<=fine<128 and -96<rel<96
    h=struct.pack('<I',263)+name.encode('latin-1','replace')[:22].ljust(22,b'\0')+b'\0'+struct.pack('<HI',1,40)+bytes(96)+bytes(48)+bytes(48)
    h+=bytes(14)+struct.pack('<H',0)+bytes(263-len(h)-14-2)
    assert len(h)==263,len(h)
    sh=sample_header(sample,name,rel,fine, max(1,min(64,int(vol))))
    assert len(sh)==40,len(sh)
    return h+sh+delta8(sample)


def load_sample(z,pad,srcid,notes):
    """Read Koala's lossless 24-bit WAV, trim pad, choose safe mono mix,
    resample with a linear-phase antialias filter; no muffling stem filters.
    """
    src=f'sampler/{srcid}.wav'
    if src not in z.namelist():raise RuntimeError(f'used pad {pad["pad"]} has no sample {src}')
    wav,rate=sf.read(io.BytesIO(z.read(src)),dtype='float32',always_2d=True)
    if rate!=INPUT_RATE:raise RuntimeError(f'expected 48kHz {src}, got {rate}')
    start=max(0,min(len(wav),int(pad.get('start',0))))
    end=max(start+1,min(len(wav),int(pad.get('end',len(wav)))))
    wav=wav[start:end]
    if wav.shape[1]>1:
        L,R=wav[:,0],wav[:,1]
        corr=float(np.dot(L,R) / max(1e-16,np.linalg.norm(L)*np.linalg.norm(R)))
        # Prevent phase cancellation from heavy Koala stereo effects; keep the
        # stronger original channel if the pair is mostly antiphase.
        wav=(L if np.sqrt(np.mean(L*L))>=np.sqrt(np.mean(R*R)) else R) if corr < -.2 else ((L+R)*.5)
    else:wav=wav[:,0]
    # Sample data is purposely NOT low-passed to 3k or heavily compressed.
    mono=signal.resample_poly(wav,1,INPUT_RATE//OUTPUT_RATE).astype(np.float32)
    peak=float(np.max(np.abs(mono)))
    if peak>1e-8:mono=np.clip(mono/peak*.97,-1,1)
    fade=min(96,len(mono)//8)
    if fade>0:mono[-fade:]*=np.linspace(1.,0.,fade,dtype=np.float32)
    return np.round(mono*127).astype(np.int8),{'sample_id':srcid,'sample_seconds':len(wav)/rate,'sample_output_bytes':len(mono),'peak_original':peak}


def pack_pattern(rows):
    data=bytearray()
    for row in rows:
        assert len(row)==XM_CHANNELS
        for note,inst,vol in row:
            if note:
                data+=bytes((note,inst,vol,0,0))
            else:
                data.append(0x80)
    return struct.pack('<IBHH',9,0,len(rows),len(data))+data


def make(source,destination,reportfile=None):
    src=pathlib.Path(source)
    with zipfile.ZipFile(src) as z:
        sq=json.loads(z.read('sequence.json'))
        sp=json.loads(z.read('sampler/sampler.json'))
        ps={int(p['pad']):p for p in sp['pads']}
        sections=[]
        used=collections.Counter(); allnotes=[]; bars_so_far=0; sec_details=[]
        for i,seq in enumerate(sq['sequences']):
            pat=seq.get('noteSequence') and seq['noteSequence'].get('pattern')
            if not pat or not pat.get('notes'):continue
            nbar=int(pat['numBars']);notes=pat['notes'];sections.append(i)
            sec_details.append({'index':i,'bars':nbar,'notes':len(notes),'starts_at_bar':bars_so_far})
            for event in notes:
                padno=int(event['num'])
                if padno not in ps:raise RuntimeError(f'Koala note references missing pad {padno}')
                used[padno]+=1
                beat=bars_so_far*4+float(event['timeOffset'])/4096
                row=beat_to_row(beat, max(.5,min(.75,float(sq.get('swing',.5)))))
                length=float(event['length'])/4096 * ROWS_PER_BEAT
                allnotes.append((row,padno,int(round(float(event.get('pitch',0)))),float(event.get('vel',110)),length))
            bars_so_far+=nbar
        assert sections==[0,1,2,3,4,5,6], f'unexpected Koala arrangement: {sections}'
        assert len(allnotes)>8000 and bars_so_far==206
        padkeys=sorted(used)
        assert len(padkeys)+1<=32
        inst={key:i+1 for i,key in enumerate(padkeys)}
        pad_audio={};infos={}
        for pn in padkeys:
            p=ps[pn]
            pcm,info=load_sample(z,p,int(p['sampleId']),used[pn])
            infos[pn]={**info,'pad':pn,'name':next((v['metadata']['originalPath'].split('/')[-1] for v in sp['samples'] if v['id']==int(p['sampleId'])),'sample'), 'events':used[pn],'pitch':p.get('pitch',0),'gain':p.get('vol',1),'oneshot':p.get('oneshot','false')}
            pad_audio[pn]=pcm

    # Assign up to 16 independent sample voices per row. We retain simultaneous
    # notes, and prefer free/expired voices to preserve melodic tails. The GBA
    # engine plays XM notes to sample end; a chosen voice may be re-triggered.
    # Instruments (pads) remain distinct to preserve individual pitch/timbre.
    allnotes.sort(key=lambda a:(a[0],-int(a[1] in (5,6,4,8)), -a[3],a[1]))
    note_by_row=collections.defaultdict(list)
    for note in allnotes:note_by_row[note[0]].append(note)
    free_until=[-1]*XM_CHANNELS
    channel_pad=[None]*XM_CHANNELS
    grid=[[(0,0,0)]*XM_CHANNELS for _ in range(bars_so_far*ROWS_PER_BAR)]
    dropped=[];steals=0;kept=0
    for row in sorted(note_by_row):
        if row>=len(grid):continue
        same=note_by_row[row]
        occupied=[]
        for item in same:
            row,pn,pitch,vel,length=item
            choices=[i for i in range(XM_CHANNELS) if i not in occupied]
            if not choices:
                dropped.append((row,pn));continue
            # Reuse same pad channel if the prior voice is over. Otherwise
            # steal the least remaining sample time, balancing channel wear.
            def score(i):
                busy=free_until[i]>row
                prev=channel_pad[i]
                samepad=(prev==pn)
                return (busy, 0 if samepad else 1, max(0,free_until[i]-row),i)
            ch=min(choices,key=score)
            if free_until[ch]>row:steals+=1
            occupied.append(ch)
            koala=ps[pn]
            actual=float(koala.get('pitch',0))+pitch
            rate=2**(actual/12)
            lifespan=infos[pn]['sample_seconds']/rate * sq['bpm']*ROWS_PER_BEAT/60
            # Koala's note duration is relevant to the channel allocation even
            # though XM's GBA player has no note-off. Use conservative tails.
            free_until[ch]=row+max(1,int(min(lifespan,length+max(3,.5*ROWS_PER_BEAT))))
            channel_pad[ch]=pn
            note_num=max(1,min(96,49+pitch))
            gain=float(koala.get('vol',1))
            vol=int(max(1,min(64, round(64*min(1,vel/127)))))
            grid[row][ch]=(note_num,inst[pn],0x10+vol)
            kept+=1

    # 206 one-bar patterns, order stays under FastTracker's 256-item cap.
    patterns=[pack_pattern(grid[b*ROWS_PER_BAR:(b+1)*ROWS_PER_BAR]) for b in range(bars_so_far)]
    order=bytes(range(bars_so_far))
    title=b'CORPORATIONS WINNER'[:20]
    header=(b'Extended Module: '+title.ljust(20,b' ')+b'\x1a'+b'Koala project import'.ljust(20,b'\0')[:20]
            +struct.pack('<H',0x0104)+struct.pack('<I',276)
            +struct.pack('<8H',len(order),0,XM_CHANNELS,len(patterns),len(padkeys),1,XM_SPEED,XM_BPM)
            +order.ljust(256,b'\0'))
    assert len(header)==336
    bank=[]
    for pn in padkeys:
        p=ps[pn]
        bank.append(instrument_bytes('KOALA '+str(pn),pad_audio[pn],float(p.get('pitch',0)),64*min(1.,max(.1,float(p.get('vol',1))))))
    dst=pathlib.Path(destination);dst.parent.mkdir(parents=True,exist_ok=True)
    dst.write_bytes(header+b''.join(patterns)+b''.join(bank))
    duration=bars_so_far*ROWS_PER_BAR*XM_SPEED*2.5/XM_BPM
    report={'source_sha256':__import__('hashlib').sha256(src.read_bytes()).hexdigest(),
            'name':sq.get('name','Winner'), 'original_koala_bpm':sq['bpm'],'original_swing':sq.get('swing'),
            'xm_bpm':XM_BPM,'xm_speed':XM_SPEED,'rows_per_bar':ROWS_PER_BAR,'sections':sec_details,
            'total_bars':bars_so_far,'duration_seconds':round(duration,3),'notes_in_original':len(allnotes),
            'notes_imported':kept,'notes_dropped':len(dropped),'stolen_active_channels':steals,
            'sample_pads':len(padkeys),'sample_bytes':sum(len(x) for x in pad_audio.values()),
            'xm_bytes':dst.stat().st_size,'used_pads':infos,'skipped_samples':[417,437,450],
            'limitations':['Koala FX bus flanger/delay/reverb and master compressor not recreated in GBA XM player',
                           'GBA XM player ignores envelopes, panning, note-off and effects',
                           'tracker audio is mono 8-bit sample data at 16kHz, resampled into 18,157Hz output',
                           'mixer has 16 channels; a busy sustained voice may be replaced by a later note']}
    if reportfile:pathlib.Path(reportfile).write_text(json.dumps(report,indent=2)+'\n',encoding='utf8')
    return report


if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('project',help='original Koala project, usually Winner.koala')
    ap.add_argument('--output',default='tools/winner_koala.xm')
    ap.add_argument('--report',default='tools/winner_koala_import_report.json')
    args=ap.parse_args()
    result=make(args.project,args.output,args.report)
    print(f"Koala Winner: {result['notes_imported']}/{result['notes_in_original']} original notes, "+
          f"{len(result['sections'])} sections / {result['total_bars']} bars, {result['duration_seconds']:.1f}s, "+
          f"{result['sample_pads']} original pads, {result['xm_bytes']/1024:.1f} KiB XM")
