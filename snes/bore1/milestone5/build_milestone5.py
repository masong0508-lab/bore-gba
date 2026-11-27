#!/usr/bin/env python3
"""Two resident native soundtracks + eight independent campaigns and four arena maps.
The 65816 uploads BRR+SPC700 program in bounded 128-byte chunks after each
frame's graphics writes. No external media or enhanced cartridge is required.
"""
from pathlib import Path
import struct, hashlib, json, sys
import build_milestone4 as m4
ROOT=Path(__file__).resolve().parent
STATE,POS,WAIT,COUNT,MUTE=72,74,76,77,78
CHUNK=128
NATIVEOK=33

def build(first=None, second=None):
    first=(ROOT/'track_0.bin').read_bytes() if first is None else first
    second=(ROOT/'track_31.bin').read_bytes() if second is None else second
    assert 0<len(first)<0xfe00 and 0<len(second)<0xfe00
    s=m4.source_with_m4(len(first))
    def rep(old,new):
        nonlocal s
        assert s.count(old)==1,(old,s.count(old));s=s.replace(old,new)
    # Start a background-in-the-main-loop upload, not an unbounded reset wait.
    rep("call('load_game');call('native_upload')","call('load_game');call('native_request')")
    rep("L('no_hud');jmp('frame')","L('no_hud');call('native_step');jmp('frame')")
    rep("ld(EL);E(0x29,0x30);R(0xf0,'no_track');ld(MSU);R(0xf0,'no_track');ld(TRACK);E(0x49,3);st(TRACK);call('music');L('no_track');E(0x60)","ld(EL);E(0x29,0x30);R(0xf0,'no_track');ld(72);R(0xd0,'no_track');ld(TRACK);E(0x49,3);st(TRACK);call('native_request');ld(MSU);R(0xf0,'no_track');call('music');L('no_track');E(0x60)")
    # Don't let MSU control collide with the SPC upload data port.
    rep("reg(0x2007,3);ld(33);R(0xf0,'media_finish');reg(0x2141,1);R(0x80,'media_finish');L('media_error');ld(33);R(0xf0,'media_finish');reg(0x2141,0);L('media_finish');E(0x64,PENDING)","reg(0x2007,3);setdp(78,1);R(0x80,'media_finish');L('media_error');E(0x64,78);L('media_finish');ld(33);R(0xf0,'media_no_native');ld(78);sta(0x2141);L('media_no_native');E(0x64,PENDING);dirty()")
    rep("L('music');reg(0x2007,0)","L('music');E(0x64,78);reg(0x2007,0)")
    # Keep audio status in the empty right of row 1 (task text occupies left).
    rep("addr(1*32+25);ld(33);R(0xf0,'audio_no');imm(tile('O'));R(0x80,'audio_ind');L('audio_no');imm(tile('-'));L('audio_ind');sta(0x2118);W(0x9c,0x2119);reg(0x2118,tile('N'));W(0x9c,0x2119)","addr(1*32+25);ld(72);R(0xf0,'audio_not_loading');imm(tile('L'));R(0x80,'audio_ind');L('audio_not_loading');ld(33);R(0xf0,'audio_no');ld(78);R(0xf0,'audio_native');imm(tile('M'));R(0x80,'audio_ind');L('audio_native');imm(tile('N'));R(0x80,'audio_ind');L('audio_no');imm(tile('-'));L('audio_ind');sta(0x2118);W(0x9c,0x2119);ld(TRACK);E(0x18,0x69,tile('0'));sta(0x2118);W(0x9c,0x2119)")
    start=s.index("    L('native_upload');");end=s.index("    L('campaign_x');",start)
    loader=f'''    # State 1: wait for IPL; state 2: transfer bounded chunks; state 3:
    # wait for driver ready. State 0 is idle. Every wait has a finite budget.
    L('native_request');ld(33);R(0xf0,'native_request_init');reg(0x2141,0x80);L('native_request_init');E(0x64,33,0x64,74,0x64,75,0x64,78);setdp(72,1);setdp(76,240);dirty();E(0x60)
    L('native_step');ld(72);R(0xd0,'native_busy');E(0x60);L('native_busy');cmp(1);R(0xd0,'native_not_ipl');jmp('native_ipl');L('native_not_ipl');cmp(2);R(0xd0,'native_not_transfer');jmp('native_transfer');L('native_not_transfer');jmp('native_ready')
    L('native_ipl');W(0xad,0x2140);cmp(0xaa);R(0xd0,'native_wait');W(0xad,0x2141);cmp(0xbb);R(0xd0,'native_wait');reg(0x2142,0);reg(0x2143,2);reg(0x2141,1);imm(0xcc);st(34);sta(0x2140);call('apu_ack');R(0xb0,'native_started');jmp('native_fail');L('native_started');setdp(72,2);E(0x60)
    L('native_wait');E(0xc6,76);R(0xd0,'native_yield');jmp('native_fail');L('native_yield');E(0x60)
    L('native_transfer');E(0xa6,74);setdp(77,{CHUNK});L('native_byte');ld(TRACK);cmp(1);R(0xd0,'native_second');W(0xe0,0x8000);R(0xb0,'native_first_hi');a.long(0xbf,0x018000);R(0x80,'native_send');L('native_first_hi');a.long(0xbf,0x020000);R(0x80,'native_send')
    L('native_second');W(0xe0,0x8000);R(0xb0,'native_second_hi');a.long(0xbf,0x038000);R(0x80,'native_send');L('native_second_hi');a.long(0xbf,0x040000)
    L('native_send');sta(0x2141);E(0x8a);st(34);sta(0x2140);call('apu_ack');R(0xb0,'native_next');jmp('native_fail');L('native_next');E(0xe8);ld(TRACK);cmp(1);R(0xd0,'native_len2');W(0xe0,{len(first)});R(0x80,'native_check_end');L('native_len2');W(0xe0,{len(second)});L('native_check_end');R(0xf0,'native_done_bytes');E(0xc6,77);R(0xd0,'native_byte');E(0x86,74,0x60)
    L('native_done_bytes');reg(0x2142,0);reg(0x2143,2);reg(0x2141,0);E(0x8a,0x18,0x69,2);sta(0x2140);setdp(72,3);setdp(76,240);E(0x60)
    L('native_ready');W(0xad,0x2140);cmp(0x42);R(0xf0,'native_success');jmp('native_wait');L('native_success');nz(33);E(0x64,72);ld(78);sta(0x2141);dirty();E(0x60)
    L('native_fail');E(0x64,72,0x64,33);dirty();E(0x60)
    L('apu_ack');W(0xa0,0x1000);L('apu_ackloop');W(0xad,0x2140);E(0xc5,34);R(0xf0,'apu_ack_ok');E(0x88);R(0xd0,'apu_ackloop');E(0x18,0x60);L('apu_ack_ok');E(0x38,0x60)
'''
    s=s[:start]+loader+s[end:]
    rep("b'BORE 1 CAMPAIGNS M4'","b'BORE 1 NATIVE M5'")
    rep("rom=bytearray(0x20000);rom[:len(program)]=program;rom[0x8000:0x8000+len(AUDIO)]=AUDIO","rom=bytearray(0x40000);rom[:len(program)]=program;rom[0x8000:0x8000+len(AUDIO)]=AUDIO;rom[0x18000:0x18000+len(AUDIO2)]=AUDIO2")
    rep("bytes([0x20,2,7,1,1,0x33,1])","bytes([0x20,2,8,1,1,0x33,1])")
    ns={'__name__':'bore_m5_generated','__file__':str(ROOT/'build_milestone2.py'),'AUDIO':first,'AUDIO2':second}
    exec(compile(s,'build_milestone5.py','exec'),ns);rom,labels=ns['build']()
    assert rom[0x8000:0x8000+len(first)]==first and rom[0x18000:0x18000+len(second)]==second
    return rom,labels
if __name__=='__main__':
    rom,labels=build();out=Path(sys.argv[1] if len(sys.argv)>1 else ROOT/'bore1-m5.sfc');out.write_bytes(rom)
    print(json.dumps(dict(bytes=len(rom),sha256=hashlib.sha256(rom).hexdigest(),chunk=CHUNK,tracks=['THE DIPPER MAN','THE TICKING BOMB'],campaigns=dict(zip(m4.CAMPAIGNS,m4.LIMITS)),reset=hex(labels['reset'])),indent=2))
